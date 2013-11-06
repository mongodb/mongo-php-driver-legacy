/**
 *  Copyright 2009-2013 10gen, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <php.h>
#include <zend_exceptions.h>
#include "mcon/io.h"
#include "mcon/manager.h"
#include "mcon/utils.h"

#include "php_mongo.h"
#include "bson.h"
#include "db.h"
#include "collection.h"
#include "util/log.h"
#include "log_stream.h"
#include "cursor_shared.h"

/* externs */
extern int le_cursor_list;

extern zend_class_entry *mongo_ce_CursorException;
extern zend_class_entry *mongo_ce_CursorTimeoutException;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

/* {{{ mongo_cursor list helpers */
static void kill_cursor_le(cursor_node *node, mongo_connection *con, zend_rsrc_list_entry *le TSRMLS_DC);


void php_mongo_cursor_free_le(void *val, int type TSRMLS_DC)
{
	zend_rsrc_list_entry *le;

	/* This should work if le->ptr is null or non-null */
	if (zend_hash_find(&EG(persistent_list), "cursor_list", strlen("cursor_list") + 1, (void**)&le) == SUCCESS) {
		cursor_node *current;

		current = le->ptr;

		while (current) {
			cursor_node *next = current->next;

			if (type == MONGO_CURSOR) {
				mongo_cursor *cursor = (mongo_cursor*)val;

				if (current->cursor_id == cursor->cursor_id && cursor->connection != NULL && current->socket == cursor->connection->socket) {
					/* If the cursor_id is 0, the db is out of results anyway */
					if (current->cursor_id == 0) {
						php_mongo_free_cursor_node(current, le);
					} else {
						kill_cursor_le(current, cursor->connection, le TSRMLS_CC);

						/* If the connection is closed before the cursor is
						 * destroyed, the cursor might try to fetch more
						 * results with disasterous consequences.  Thus, the
						 * cursor_id is set to 0, so no more results will be
						 * fetched.
						 *
						 * This might not be the most elegant solution, since
						 * you could fetch 100 results, get the first one,
						 * close the connection, get 99 more, and suddenly not
						 * be able to get any more.  Not sure if there's a
						 * better one, though. I guess the user can call dead()
						 * on the cursor. */
						cursor->cursor_id = 0;
					}
					if (cursor->connection) {
						mongo_deregister_callback_from_connection(cursor->connection, cursor);
						cursor->connection = NULL;
					}


					/* only one cursor to be freed */
					break;
				}
			}

			current = next;
		}
	}
}

int php_mongo_create_le(mongo_cursor *cursor, char *name TSRMLS_DC)
{
	zend_rsrc_list_entry *le;
	cursor_node *new_node;

	new_node = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
	new_node->cursor_id = cursor->cursor_id;
	if (cursor->connection) {
		new_node->socket = cursor->connection->socket;
	} else {
		new_node->socket = 0;
	}
	new_node->next = new_node->prev = 0;

	/*
	 * 3 options:
	 *   - le doesn't exist
	 *   - le exists and is null
	 *   - le exists and has elements
	 * In case 1 & 2, we want to create a new le ptr, otherwise we want to append
	 * to the existing ptr.
	 */
	if (zend_hash_find(&EG(persistent_list), name, strlen(name) + 1, (void**)&le) == SUCCESS) {
		cursor_node *current = le->ptr;
		cursor_node *prev = 0;

		if (current == 0) {
			le->ptr = new_node;
			return 0;
		}

		do {
			/* If we find the current cursor in the cursor list, we don't need
			 * another dtor for it so unlock the mutex & return. */
			if (current->cursor_id == cursor->cursor_id && cursor->connection && current->socket == cursor->connection->socket) {
				pefree(new_node, 1);
				return 0;
			}

			prev = current;
			current = current->next;
		} while (current);

		/* We didn't find the cursor so we add it to the list. prev is pointing
		 * to the tail of the list, current is pointing to null. */
		prev->next = new_node;
		new_node->prev = prev;
	} else {
		zend_rsrc_list_entry new_le;

		new_le.ptr = new_node;
		new_le.type = le_cursor_list;
		new_le.refcount = 1;
		zend_hash_add(&EG(persistent_list), name, strlen(name) + 1, &new_le, sizeof(zend_rsrc_list_entry), NULL);
	}

	return 0;
}

static int cursor_list_pfree_helper(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	cursor_node *node = (cursor_node*)rsrc->ptr;

	if (!node) {
		return 0;
	}

	while (node->next) {
		cursor_node *temp = node;
		node = node->next;
		pefree(temp, 1);
	}
	pefree(node, 1);

	return 0;
}

void php_mongo_cursor_list_pfree(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	cursor_list_pfree_helper(rsrc TSRMLS_CC);
}

void php_mongo_free_cursor_node(cursor_node *node, zend_rsrc_list_entry *le)
{
	if (node->prev) {
		/*
		 * [node1][<->][NODE2][<->][node3]
		 *   [node1][->][node3]
		 *   [node1][<->][node3]
		 *
		 * [node1][<->][NODE2]
		 *   [node1]
		 */
		node->prev->next = node->next;
		if (node->next) {
			node->next->prev = node->prev;
		}
	} else {
		/*
		 * [NODE2][<->][node3]
		 *   le->ptr = node3
		 *   [node3]
		 *
		 * [NODE2]
		 *   le->ptr = 0
		 */
		le->ptr = node->next;
		if (node->next) {
			node->next->prev = 0;
		}
	}

	pefree(node, 1);
}

void php_mongo_kill_cursor(mongo_connection *con, int64_t cursor_id TSRMLS_DC)
{
	char quickbuf[128];
	mongo_buffer buf;
	char *error_message;

	buf.pos = quickbuf;
	buf.start = buf.pos;
	buf.end = buf.start + 128;

	php_mongo_write_kill_cursors(&buf, cursor_id, MONGO_DEFAULT_MAX_MESSAGE_SIZE TSRMLS_CC);
#if MONGO_PHP_STREAMS
	mongo_log_stream_killcursor(con, cursor_id TSRMLS_CC);
#endif

	if (MonGlo(manager)->send(con, NULL, buf.start, buf.pos - buf.start, (char**) &error_message) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Couldn't kill cursor %lld: %s", (long long int) cursor_id, error_message);
		free(error_message);
	}
}

/* Tell the database to destroy its cursor */
static void kill_cursor_le(cursor_node *node, mongo_connection *con, zend_rsrc_list_entry *le TSRMLS_DC)
{
	/* If the cursor_id is 0, the db is out of results anyway. */
	if (node->cursor_id == 0) {
		php_mongo_free_cursor_node(node, le);
		return;
	}

	mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_WARN, "Killing unfinished cursor %ld", node->cursor_id);

	php_mongo_kill_cursor(con, node->cursor_id TSRMLS_CC);

	/* Free this cursor/link pair */
	php_mongo_free_cursor_node(node, le);
}

void php_mongo_cursor_free(void *object TSRMLS_DC)
{
	mongo_cursor *cursor = (mongo_cursor*)object;

	if (cursor) {
		if (cursor->cursor_id != 0) {
			php_mongo_cursor_free_le(cursor, MONGO_CURSOR TSRMLS_CC);
		} else if (cursor->connection) {
			mongo_deregister_callback_from_connection(cursor->connection, cursor);
		}

		if (cursor->current) {
			zval_ptr_dtor(&cursor->current);
		}

		if (cursor->query) {
			zval_ptr_dtor(&cursor->query);
		}
		if (cursor->fields) {
			zval_ptr_dtor(&cursor->fields);
		}

		if (cursor->buf.start) {
			efree(cursor->buf.start);
		}
		if (cursor->ns) {
			efree(cursor->ns);
		}

		if (cursor->zmongoclient) {
			zval_ptr_dtor(&cursor->zmongoclient);
		}

		mongo_read_preference_dtor(&cursor->read_pref);

		zend_object_std_dtor(&cursor->std TSRMLS_CC);

		efree(cursor);
	}
}
/* }}} */

/* {{{ Cursor related read/write functions */

/*
 * This method reads the message header for a database response
 * It returns failure or success and throws an exception on failure.
 *
 * Returns:
 * 0 on success
 * -1 on failure, but not critical enough to throw an exception
 * 1.. on failure, and throw an exception. The return value is the error code
 */
signed int php_mongo_get_cursor_header(mongo_connection *con, mongo_cursor *cursor, char **error_message TSRMLS_DC)
{
	int status = 0;
	int num_returned = 0;
	char buf[REPLY_HEADER_LEN];
	mongoclient *client;

	php_mongo_log(MLOG_IO, MLOG_FINE TSRMLS_CC, "getting cursor header");

	PHP_MONGO_GET_MONGOCLIENT_FROM_CURSOR(cursor);
	status = client->manager->recv_header(con, &client->servers->options, cursor->timeout, buf, REPLY_HEADER_LEN, error_message);
	if (status < 0) {
		/* Read failed, error message populated by recv_header */
		return abs(status);
	} else if (status < INT_32*4) {
		*error_message = malloc(256);
		snprintf(*error_message, 256, "couldn't get full response header, got %d bytes but expected atleast %d", status, INT_32*4);
		return 4;
	}

	/* switch the byte order, if necessary */
	cursor->recv.length = MONGO_32(*(int*)buf);

	/* make sure we're not getting crazy data */
	if (cursor->recv.length == 0) {
		*error_message = strdup("No response from the database");
		return 5;
	} else if (cursor->recv.length < REPLY_HEADER_SIZE) {
		*error_message = malloc(256);
		snprintf(*error_message, 256, "bad response length: %d, did the db assert?", cursor->recv.length);
		return 6;
	}

	cursor->recv.request_id  = MONGO_32(*(int*)(buf + INT_32));
	cursor->recv.response_to = MONGO_32(*(int*)(buf + INT_32*2));
	cursor->recv.op          = MONGO_32(*(int*)(buf + INT_32*3));
	cursor->flag             = MONGO_32(*(int*)(buf + INT_32*4));
	cursor->cursor_id        = MONGO_64(*(int64_t*)(buf + INT_32*5));
	cursor->start            = MONGO_32(*(int*)(buf + INT_32*5 + INT_64));
	num_returned             = MONGO_32(*(int*)(buf + INT_32*6 + INT_64));

#if MONGO_PHP_STREAMS
	mongo_log_stream_response_header(con, cursor TSRMLS_CC);
#endif

	/* TODO: find out what this does */
	if (cursor->recv.response_to > MonGlo(response_num)) {
		MonGlo(response_num) = cursor->recv.response_to;
	}

	/* cursor->num is the total of the elements we've retrieved (elements
	 * already iterated through + elements in db response but not yet iterated
	 * through) */
	cursor->num += num_returned;

	/* create buf */
	cursor->recv.length -= REPLY_HEADER_LEN;

	return 0;
}

/* Reads a cursors body
 * Returns -31 on failure, -80 on timeout, -32 on EOF, or an int indicating the number of bytes read */
int php_mongo_get_cursor_body(mongo_connection *con, mongo_cursor *cursor, char **error_message TSRMLS_DC)
{
	mongoclient *client;
		
	PHP_MONGO_GET_MONGOCLIENT_FROM_CURSOR(cursor);
	php_mongo_log(MLOG_IO, MLOG_FINE TSRMLS_CC, "getting cursor body");

	if (cursor->buf.start) {
		efree(cursor->buf.start);
	}

	cursor->buf.start = (char*)emalloc(cursor->recv.length);
	cursor->buf.end = cursor->buf.start + cursor->recv.length;
	cursor->buf.pos = cursor->buf.start;

	/* finish populating cursor */
	return MonGlo(manager)->recv_data(con, &client->servers->options, cursor->timeout, cursor->buf.pos, cursor->recv.length, error_message);
}

/* Cursor helper function */
int php_mongo_get_reply(mongo_cursor *cursor TSRMLS_DC)
{
	int   status;
	char *error_message = NULL;

	php_mongo_log(MLOG_IO, MLOG_FINE TSRMLS_CC, "getting reply");

	status = php_mongo_get_cursor_header(cursor->connection, cursor, (char**) &error_message TSRMLS_CC);
	if (status == -1 || status > 0) {
		zend_class_entry *exception_ce;

		if (status == 2 || status == 80) {
			exception_ce = mongo_ce_CursorTimeoutException;
		} else {
			exception_ce = mongo_ce_CursorException;
		}

		php_mongo_cursor_throw(exception_ce, cursor->connection, status TSRMLS_CC, "%s", error_message);
		free(error_message);
		return FAILURE;
	}

	/* Check that this is actually the response we want */
	if (cursor->send.request_id != cursor->recv.response_to) {
		php_mongo_log(MLOG_IO, MLOG_WARN TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);

		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 9 TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);
		return FAILURE;
	}

	if (php_mongo_get_cursor_body(cursor->connection, cursor, (char **) &error_message TSRMLS_CC) < 0) {
#ifdef WIN32
		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 12 TSRMLS_CC, "WSA error getting database response %s (%d)", error_message, WSAGetLastError());
#else
		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 12 TSRMLS_CC, "error getting database response %s (%s)", error_message, strerror(errno));
#endif
		free(error_message);
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ Cursor option setting */
/* Returns 1 on success, and 0 (raising an exception) on failure */
int php_mongo_cursor_add_option(mongo_cursor *cursor, char *key, zval *value TSRMLS_DC)
{
	zval *query;

	if (cursor->started_iterating) {
		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 0 TSRMLS_CC, "cannot modify cursor after beginning iteration");
		return 0;
	}

	php_mongo_make_special(cursor);
	query = cursor->query;
	add_assoc_zval(query, key, value);
	zval_add_ref(&value);

	return 1;
}

void php_mongo_cursor_set_limit(mongo_cursor *cursor, long limit)
{
	cursor->limit = limit;
}

void php_mongo_cursor_force_long_as_object(mongo_cursor *cursor)
{
	cursor->cursor_options |= MONGO_CURSOR_OPT_LONG_AS_OBJECT;
}

void php_mongo_cursor_force_command_cursor(mongo_cursor *cursor)
{
	cursor->cursor_options |= MONGO_CURSOR_OPT_CMD_CURSOR;
}

void php_mongo_cursor_force_primary(mongo_cursor *cursor)
{
	cursor->cursor_options |= MONGO_CURSOR_OPT_FORCE_PRIMARY;
}
/* }}} */

/* {{{ Utility functions */

/* This function encapsulates a simple query in an array where the query is 
 * added as the $query element. This new array also allows other options to be
 * set, such as limit and skip. */
void php_mongo_make_special(mongo_cursor *cursor)
{
	zval *temp;

	if (cursor->special) {
		return;
	}

	cursor->special = 1;

	temp = cursor->query;
	MAKE_STD_ZVAL(cursor->query);
	array_init(cursor->query);
	add_assoc_zval(cursor->query, "$query", temp);
}

/* This function throws an exception if none is set, and automatically adds the
 * hostname if available. */
zval* php_mongo_cursor_throw(zend_class_entry *exception_ce, mongo_connection *connection, int code TSRMLS_DC, char *format, ...)
{
	zval *e;
	va_list arg;
	char *host, *message;

	if (EG(exception)) {
		return EG(exception);
	}

	/* Construct message */
	va_start(arg, format);
	message = malloc(1024);
	vsnprintf(message, 1024, format, arg);
	va_end(arg);

	if (connection) {
		host = mongo_server_hash_to_server(connection->hash);
		e = zend_throw_exception_ex(exception_ce, code TSRMLS_CC, "%s: %s", host, message);
		zend_update_property_string(exception_ce, e, "host", strlen("host"), host TSRMLS_CC);
		free(host);
	} else {
		e = zend_throw_exception(exception_ce, message, code TSRMLS_CC);
	}

	free(message);

	return e;
}

/* Returns whether a passed in namespace is a valid one */
int php_mongo_is_valid_namespace(char *ns, int ns_len)
{
	char *dot;

	dot = strchr(ns, '.');

	if (ns_len < 3 || dot == NULL || ns[0] == '.' || ns[ns_len-1] == '.') {
		return 0;
	}
	return 1;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
