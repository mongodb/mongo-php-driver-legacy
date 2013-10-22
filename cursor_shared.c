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
#include "mcon/io.h"
#include "mcon/manager.h"
#include "mcon/utils.h"

#include "php_mongo.h"
#include "bson.h"
#include "db.h"
#include "cursor.h"
#include "collection.h"
#include "util/log.h"
#include "log_stream.h"
#include "cursor_shared.h"

/* externs */
extern zend_class_entry *mongo_ce_CursorException;
extern zend_class_entry *mongo_ce_CursorTimeoutException;
ZEND_EXTERN_MODULE_GLOBALS(mongo)

/*
 * Cursor related read/write functions
 */
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

		mongo_cursor_throw(exception_ce, cursor->connection, status TSRMLS_CC, "%s", error_message);
		free(error_message);
		return FAILURE;
	}

	/* Check that this is actually the response we want */
	if (cursor->send.request_id != cursor->recv.response_to) {
		php_mongo_log(MLOG_IO, MLOG_WARN TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);

		mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 9 TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);
		return FAILURE;
	}

	if (php_mongo_get_cursor_body(cursor->connection, cursor, (char **) &error_message TSRMLS_CC) < 0) {
#ifdef WIN32
		mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 12 TSRMLS_CC, "WSA error getting database response %s (%d)", error_message, WSAGetLastError());
#else
		mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 12 TSRMLS_CC, "error getting database response %s (%s)", error_message, strerror(errno));
#endif
		free(error_message);
		return FAILURE;
	}

	return SUCCESS;
}

/* Returns 1 on success, and 0 (raising an exception) on failure */
int php_mongo_cursor_add_option(mongo_cursor *cursor, char *key, zval *value TSRMLS_DC)
{
	zval *query;

	if (cursor->started_iterating) {
		mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 0 TSRMLS_CC, "cannot modify cursor after beginning iteration");
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
