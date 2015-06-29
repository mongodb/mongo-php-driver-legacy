/**
 *  Copyright 2009-2014 MongoDB, Inc.
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
#include <zend_interfaces.h>
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

extern zend_class_entry *mongo_ce_Cursor, *mongo_ce_CommandCursor;
extern zend_class_entry *mongo_ce_CursorException;
extern zend_class_entry *mongo_ce_CursorTimeoutException;
extern zend_class_entry *mongo_ce_DuplicateKeyException;
extern zend_class_entry *mongo_ce_ExecutionTimeoutException;
extern zend_class_entry *mongo_ce_WriteConcernException;
extern zend_class_entry *mongo_ce_Int64;
extern zend_class_entry *mongo_ce_Exception, *mongo_ce_CursorException;

zend_class_entry *mongo_ce_CursorInterface = NULL;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

void php_mongo_kill_cursor(mongo_connection *con, int64_t cursor_id TSRMLS_DC)
{
	char quickbuf[128];
	mongo_buffer buf;
	char *error_message;

	buf.pos = quickbuf;
	buf.start = buf.pos;
	buf.end = buf.start + 128;

	mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_WARN, "Killing unfinished cursor %ld", cursor_id);

	php_mongo_write_kill_cursors(&buf, cursor_id, MONGO_DEFAULT_MAX_MESSAGE_SIZE TSRMLS_CC);

	mongo_log_stream_killcursor(con, cursor_id TSRMLS_CC);

	if (MonGlo(manager)->send(con, NULL, buf.start, buf.pos - buf.start, (char**) &error_message) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Couldn't kill cursor %lld: %s", (long long int) cursor_id, error_message);
		free(error_message);
	}
}

void php_mongo_cursor_free(void *object TSRMLS_DC)
{
	mongo_cursor *cursor = (mongo_cursor*)object;

	if (cursor) {
		if (cursor->connection) {
			if (cursor->cursor_id != 0) {
				php_mongo_kill_cursor(cursor->connection, cursor->cursor_id TSRMLS_CC);
			}
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

		if (cursor->first_batch) {
			zval_ptr_dtor(&cursor->first_batch);
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

	mongo_log_stream_response_header(con, cursor TSRMLS_CC);

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

/* Cursor helper functions */
int php_mongo_cursor_mark_dead(void *callback_data)
{
	mongo_cursor *cursor = (mongo_cursor*) callback_data;

	cursor->dead = 1;
	cursor->cursor_id = 0;
	cursor->connection = NULL;

	return 1;
}

void php_mongo_cursor_clear_current_element(mongo_cursor *cursor)
{
	if (cursor->current) {
		zval_ptr_dtor(&cursor->current);
		cursor->current = NULL;
	}
}

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

/* Returns 1 when an error was found and *handled*, and it returns 0 if no
 * error situation has ocurred on the cursor. If the error is handled, then an
 * exception has been thrown as well. */
int php_mongo_handle_error(mongo_cursor *cursor TSRMLS_DC)
{
	zval **err = NULL;

	/* check for $err */
	if (
		cursor->current &&
		zend_hash_find(Z_ARRVAL_P(cursor->current), "$err", strlen("$err") + 1, (void**)&err) == SUCCESS
	) {
		zval **code_z, *exception;
		/* default error code */
		int code = 4;

		/* check for error code */
		if (zend_hash_find(Z_ARRVAL_P(cursor->current), "code", strlen("code") + 1, (void**)&code_z) == SUCCESS) {
			convert_to_long_ex(code_z);
			code = Z_LVAL_PP(code_z);
		}

		/* TODO: Determine if we need to throw MongoCursorTimeoutException
		 * or MongoWriteConcernException here, depending on the code. */
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, code TSRMLS_CC, "%s", Z_STRVAL_PP(err));
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), cursor->current TSRMLS_CC);
		php_mongo_cursor_clear_current_element(cursor);

		/* We check for "not master" error codes. The source of those codes
		 * is at https://github.com/mongodb/mongo/blob/master/docs/errors.md
		 *
		 * We should kill the connection so the next request doesn't do the
		 * same wrong thing.
		 *
		 * Note: We need to mark the cursor as failed _after_ prepping the
		 * exception, otherwise the exception won't include the servername
		 * it hit for example. */
		if (code == 10107 || code == 13435 || code == 13436 || code == 10054 || code == 10056 || code == 10058) {
			php_mongo_cursor_failed(cursor TSRMLS_CC);
		}

		return 1;
	}

	if (cursor->flag & MONGO_OP_REPLY_ERROR_FLAGS) {
		if (cursor->flag & MONGO_OP_REPLY_CURSOR_NOT_FOUND) {
			php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 16336 TSRMLS_CC, "could not find cursor over collection %s", cursor->ns);
			return 1;
		}

		if (cursor->flag & MONGO_OP_REPLY_QUERY_FAILURE) {
			php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 2 TSRMLS_CC, "query failure");
			return 1;
		}

		/* Default case */
		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 29 TSRMLS_CC, "Unknown query/get_more failure");
		return 1;
	}

	return 0;
}

int php_mongo_get_more(mongo_cursor *cursor TSRMLS_DC)
{
	mongo_buffer buf;
	int          size;
	char        *error_message;
	mongoclient *client;

	size = 34 + strlen(cursor->ns);
	CREATE_BUF(buf, size);


	if (cursor->connection == NULL) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 18 TSRMLS_CC, "trying to get more, but cannot find server");
		return 0;
	}

	if (FAILURE == php_mongo_write_get_more(&buf, cursor TSRMLS_CC)) {
		efree(buf.start);
		return 0;
	}

	mongo_log_stream_getmore(cursor->connection, cursor TSRMLS_CC);

	PHP_MONGO_GET_MONGOCLIENT_FROM_CURSOR(cursor);

	if (client->manager->send(cursor->connection, &client->servers->options, buf.start, buf.pos - buf.start, (char **) &error_message) == -1) {
		efree(buf.start);

		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 1 TSRMLS_CC, "%s", error_message);
		free(error_message);
		php_mongo_cursor_failed(cursor TSRMLS_CC);
		return 0;
	}

	efree(buf.start);

	if (php_mongo_get_reply(cursor TSRMLS_CC) != SUCCESS) {
		php_mongo_cursor_failed(cursor TSRMLS_CC);
		return 0;
	}

	return 1;
}


int php_mongo_get_cursor_info_envelope(zval *document, zval **cursor TSRMLS_DC)
{
	zval **tmp;

	if (Z_TYPE_P(document) != IS_ARRAY) {
		return FAILURE;
	}

	if (zend_hash_find(Z_ARRVAL_P(document), "cursor", sizeof("cursor"), (void **)&tmp) == FAILURE) {
		return FAILURE;
	}
	if (Z_TYPE_PP(tmp) != IS_ARRAY) {
		return FAILURE;
	}

	*cursor = *tmp;
	return SUCCESS;
}

int php_mongo_get_cursor_info(zval *cursor, int64_t *cursor_id, char **ns, zval **first_batch TSRMLS_DC)
{
	zval **id = NULL, **znamespace = NULL, **first = NULL;
	zval  *id_value;

	/* Cursor ID */
	if (zend_hash_find(Z_ARRVAL_P(cursor), "id", sizeof("id"), (void **)&id) == FAILURE) {
		return FAILURE;
	}
	if (Z_TYPE_PP(id) != IS_OBJECT || Z_OBJCE_PP(id) != mongo_ce_Int64) {
		return FAILURE;
	}
	id_value = zend_read_property(mongo_ce_Int64, *id, "value", strlen("value"), NOISY TSRMLS_CC);
	if (Z_TYPE_P(id_value) != IS_STRING) {
		return FAILURE;
	}

	/* Namespace */
	if (zend_hash_find(Z_ARRVAL_P(cursor), "ns", sizeof("ns"), (void **)&znamespace) == FAILURE) {
		return FAILURE;
	}
	if (Z_TYPE_PP(znamespace) != IS_STRING) {
		return FAILURE;
	}

	/* First batch */
	if (zend_hash_find(Z_ARRVAL_P(cursor), "firstBatch", sizeof("firstBatch"), (void **)&first) == FAILURE) {
		return FAILURE;
	}
	if (Z_TYPE_PP(first) != IS_ARRAY) {
		return FAILURE;
	}

	/* Assign duplicates */
	*first_batch = *first;
	*ns = Z_STRVAL_PP(znamespace);
	*cursor_id = strtoll(Z_STRVAL_P(id_value), NULL, 10);

	return SUCCESS;
}

int php_mongo_calculate_next_request_limit(mongo_cursor *cursor)
{
	int lim_at;

	if (cursor->limit < 0) {
		return cursor->limit;
	} else if (cursor->batch_size < 0) {
		return cursor->batch_size;
	}

	lim_at = cursor->limit > cursor->batch_size ? cursor->limit - cursor->at : cursor->limit;

	if (cursor->batch_size && (!lim_at || cursor->batch_size <= lim_at)) {
		return cursor->batch_size;
	} else if (lim_at && (!cursor->batch_size || lim_at < cursor->batch_size)) {
		return lim_at;
	}

	return 0;
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

	/* Based on the status, we pick a different exception class.
	 *
	 * For specific cases we pick something else than the default
	 * mongo_ce_CursorException:
	 * - code 50, which is an operation exceeded timeout.
	 * - code 80, which is a cursor timeout.
	 * - codes 11000, 11001, 12582, which are all duplicate key exceptions
	 *
	 * Code 80 *also* comes from recv_header() (abs()) recv_data() stream
	 * handlers */
	switch (code) {
		case 50:
			exception_ce = mongo_ce_ExecutionTimeoutException;
			break;
		case 80:
			exception_ce = mongo_ce_CursorTimeoutException;
			break;
		case 11000:
		case 11001:
		case 12582:
			exception_ce = mongo_ce_DuplicateKeyException;
			break;
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

/* Splits a namespace name into the database and collection names, allocated with estrdup. */
void php_mongo_split_namespace(char *ns, char **dbname, char **cname)
{
	if (cname) {
		*cname = estrdup(ns + (strchr(ns, '.') - ns) + 1);
	}
	if (dbname) {
		*dbname = estrndup(ns, strchr(ns, '.') - ns);
	}
}
/* }}} */


/* {{{ Iteration helpers and functions */

/* Reset the cursor to clean up or prepare for another query. Removes cursor
 * from cursor list (and kills it, if necessary).  */
void php_mongo_cursor_reset(mongo_cursor *cursor TSRMLS_DC)
{
	cursor->buf.pos = cursor->buf.start;

	if (cursor->current) {
		zval_ptr_dtor(&cursor->current);
	}

	if (cursor->first_batch) {
		zval_ptr_dtor(&cursor->first_batch);
		cursor->first_batch = NULL;
	}

	if (cursor->cursor_id != 0) {
		php_mongo_kill_cursor(cursor->connection, cursor->cursor_id TSRMLS_CC);
		cursor->cursor_id = 0;
	}

	cursor->started_iterating = 0;
	cursor->current = 0;
	cursor->at = 0;
	cursor->num = 0;
	cursor->dead = 0;
	cursor->persist = 0;
	cursor->first_batch_at = 0;
	cursor->first_batch_num = 0;
	cursor->cursor_options &= ~MONGO_CURSOR_OPT_DONT_ADVANCE_ON_FIRST_NEXT;
}

/* Resets cursor and disconnects connection.  Always returns FAILURE (so it can
 * be used by functions returning FAILURE). */
int php_mongo_cursor_failed(mongo_cursor *cursor TSRMLS_DC)
{
	mongo_manager_connection_deregister(MonGlo(manager), cursor->connection);
	cursor->dead = 1;
	cursor->cursor_id = 0;
	cursor->connection = NULL;

	return FAILURE;
}
/* }}} */

/* {{{ MongoCursorInterface */
ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_batchsize, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setReadPreference, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, read_preference)
	ZEND_ARG_ARRAY_INFO(0, tags, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_timeout, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, timeoutMS)
ZEND_END_ARG_INFO()

static const zend_function_entry mongo_cursor_funcs_interface[] = {
	/* options */
	PHP_ABSTRACT_ME(MongoCursorInterface, batchSize, arginfo_batchsize)

	/* query */
	PHP_ABSTRACT_ME(MongoCursorInterface, info, arginfo_no_parameters)
	PHP_ABSTRACT_ME(MongoCursorInterface, dead, arginfo_no_parameters)
	PHP_ABSTRACT_ME(MongoCursorInterface, timeout, arginfo_timeout)

	/* read preferences */
	PHP_ABSTRACT_ME(MongoCursorInterface, getReadPreference, arginfo_no_parameters)
	PHP_ABSTRACT_ME(MongoCursorInterface, setReadPreference, arginfo_setReadPreference)

	PHP_FE_END
};

static int implement_mongo_cursor_interface_handler(zend_class_entry *interface, zend_class_entry *implementor TSRMLS_DC)
{
    if (implementor->type == ZEND_USER_CLASS &&
        !instanceof_function(implementor, mongo_ce_Cursor TSRMLS_CC) &&
        !instanceof_function(implementor, mongo_ce_CommandCursor TSRMLS_CC)
    ) {
        zend_error(E_ERROR, "MongoCursorInterface can't be implemented by user classes");
    }

    return SUCCESS;
}

void mongo_init_MongoCursorInterface(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoCursorInterface", mongo_cursor_funcs_interface);
	mongo_ce_CursorInterface = zend_register_internal_interface(&ce TSRMLS_CC);
	mongo_ce_CursorInterface->interface_gets_implemented = implement_mongo_cursor_interface_handler;
	zend_class_implements(mongo_ce_CursorInterface TSRMLS_CC, 1, zend_ce_iterator);
}

/* {{{ MongoCursorInterface MongoCursorInterface->batchSize(void)
 */
PHP_METHOD(MongoCursorInterface, batchSize)
{
	long l;
	mongo_cursor *cursor;

	PHP_MONGO_GET_CURSOR(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &l) == FAILURE) {
		return;
	}

	cursor->batch_size = l;
	RETVAL_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ boolean MongoCursorInterface->dead(void)
 */
PHP_METHOD(MongoCursorInterface, dead)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursorInterface);

	RETURN_BOOL(cursor->dead || (cursor->started_iterating && cursor->cursor_id == 0));
}
/* }}} */

/* {{{ proto array MongoCursorInterface::getReadPreference()
   Returns an array describing the read preference for this cursor. Tag sets will be included if available. */
PHP_METHOD(MongoCursorInterface, getReadPreference)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	array_init(return_value);
	add_assoc_string(return_value, "type", mongo_read_preference_type_to_name(cursor->read_pref.type), 1);
	php_mongo_add_tagsets(return_value, &cursor->read_pref);
}
/* }}} */

/* {{{ proto bool MongoCursorInterface::setReadPreference(string read_preference [, array tags ])
   Sets the read preference for this cursor */
PHP_METHOD(MongoCursorInterface, setReadPreference)
{
	char *read_preference;
	int   read_preference_len;
	mongo_cursor *cursor;
	HashTable  *tags = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|h", &read_preference, &read_preference_len, &tags) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_CURSOR(getThis());

	php_mongo_set_readpreference(&cursor->read_pref, read_preference, tags TSRMLS_CC);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ array MongoCursorInterface->info(void)
 * Return execution and connection information of the current cursor */
PHP_METHOD(MongoCursorInterface, info)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursorInterface);
	array_init(return_value);

	add_assoc_string(return_value, "ns", cursor->ns, 1);
	add_assoc_long(return_value, "limit", cursor->limit);
	add_assoc_long(return_value, "batchSize", cursor->batch_size);
	add_assoc_long(return_value, "skip", cursor->skip);
	add_assoc_long(return_value, "flags", cursor->opts);
	if (cursor->query) {
		add_assoc_zval(return_value, "query", cursor->query);
		zval_add_ref(&cursor->query);
	} else {
		add_assoc_null(return_value, "query");
	}
	if (cursor->fields) {
		add_assoc_zval(return_value, "fields", cursor->fields);
		zval_add_ref(&cursor->fields);
	} else {
		add_assoc_null(return_value, "fields");
	}

	add_assoc_bool(return_value, "started_iterating", cursor->started_iterating);

	if (cursor->started_iterating) {
		char *host;
		int   port;
		zval *id_value;

		MAKE_STD_ZVAL(id_value);
		ZVAL_NULL(id_value);
		php_mongo_handle_int64(&id_value, cursor->cursor_id, BSON_OPT_INT32_LONG_AS_OBJECT TSRMLS_CC);
		add_assoc_zval(return_value, "id", id_value);

		add_assoc_long(return_value, "at", cursor->at);
		add_assoc_long(return_value, "numReturned", cursor->num);

		if (cursor->connection) {
			add_assoc_string(return_value, "server", cursor->connection->hash, 1);

			mongo_server_split_hash(cursor->connection->hash, &host, &port, NULL, NULL, NULL, NULL, NULL);
			add_assoc_string(return_value, "host", host, 1);
			free(host);
			add_assoc_long(return_value, "port", port);
			add_assoc_string(return_value, "connection_type_desc", mongo_connection_type(cursor->connection->connection_type), 1);
		}

		if (cursor->cursor_options & MONGO_CURSOR_OPT_CMD_CURSOR) {
			add_assoc_long(return_value, "firstBatchAt", cursor->first_batch_at);
			add_assoc_long(return_value, "firstBatchNumReturned", cursor->first_batch_num);
		}
	}
}
/* }}} */


/* {{{ MongoCursor::timeout
 */
PHP_METHOD(MongoCursorInterface, timeout)
{
	long timeout;
	mongo_cursor *cursor;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &timeout) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_CURSOR(getThis());

	cursor->timeout = timeout;

	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
