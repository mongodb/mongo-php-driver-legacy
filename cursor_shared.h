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
#ifndef MONGO_CURSOR_SHARED_H
#define MONGO_CURSOR_SHARED_H 1

#include "php_mongo.h"

/* Cursor flags */
#define CURSOR_FLAG_TAILABLE      2
#define CURSOR_FLAG_SLAVE_OKAY    4
#define CURSOR_FLAG_OPLOG_REPLAY  8 /* Don't use */
#define CURSOR_FLAG_NO_CURSOR_TO 16
#define CURSOR_FLAG_AWAIT_DATA   32
#define CURSOR_FLAG_EXHAUST      64 /* Not implemented */
#define CURSOR_FLAG_PARTIAL     128

/* OP_REPLY flags */
#define MONGO_OP_REPLY_CURSOR_NOT_FOUND     0x01
#define MONGO_OP_REPLY_QUERY_FAILURE        0x02
#define MONGO_OP_REPLY_SHARD_CONFIG_STALE   0x04
#define MONGO_OP_REPLY_AWAIT_CAPABLE        0x08
#define MONGO_OP_REPLY_ERROR_FLAGS          (MONGO_OP_REPLY_CURSOR_NOT_FOUND|MONGO_OP_REPLY_QUERY_FAILURE)

/* Extension specific cursor options */
#define MONGO_CURSOR_OPT_CMD_CURSOR                 0x02
#define MONGO_CURSOR_OPT_FORCE_PRIMARY              0x04
#define MONGO_CURSOR_FIRST_BATCH_EMPTY_OKAY         0x08
#define MONGO_CURSOR_OPT_DONT_ADVANCE_ON_FIRST_NEXT 0x10

/* Macro to check whether a cursor is dead, and if so, bailout */
#define MONGO_CURSOR_CHECK_DEAD \
	if (cursor->dead) { \
		php_mongo_cursor_throw(mongo_ce_ConnectionException, NULL, 12 TSRMLS_CC, "the connection has been terminated, and this cursor is dead"); \
		return; \
	}

/* The cursor_list
 *
 * In PHP, garbage collection works via reference counting.  MongoCursor
 * contains a reference to its "parent" Mongo instance, so it increments the
 * Mongo's reference count in the constructor.
 *
 * Depending on app server/code, MongoCursor could be destroyed before or after
 * Mongo.  If Mongo is destroyed first, we want to kill all open cursors using
 * that connection before destroying the connection.  So, mongo_cursor_free_le,
 * when given a MONGO_LINK, will kill all cursors associated with that link.
 * When given a MONGO_CURSOR, it will destroy exactly that cursor (and no
 * others).  This also removes it from the cursor_list. */

void php_mongo_cursor_free(void *object TSRMLS_DC);

signed int php_mongo_get_cursor_header(mongo_connection *con, mongo_cursor *cursor, char **error_message TSRMLS_DC);
int php_mongo_get_cursor_body(mongo_connection *con, mongo_cursor *cursor, char **error_message TSRMLS_DC);
int php_mongo_handle_error(mongo_cursor *cursor TSRMLS_DC);
int php_mongo_get_more(mongo_cursor *cursor TSRMLS_DC);

int php_mongo_get_cursor_info_envelope(zval *document, zval **cursor TSRMLS_DC);
int php_mongo_get_cursor_info(zval *document, int64_t *cursor_id, char **ns, zval **first_batch TSRMLS_DC);

/* Returns the actual limit to send over the wire, based on batch size, current
 * position, and user limit */
int php_mongo_calculate_next_request_limit(mongo_cursor *cursor);

/* Tries to read the reply from the database */
int php_mongo_cursor_mark_dead(void *callback_data);
int php_mongo_get_reply(mongo_cursor *cursor TSRMLS_DC);
void php_mongo_make_special(mongo_cursor *cursor);

/* Clears the cursor's current element if it is set */
void php_mongo_cursor_clear_current_element(mongo_cursor *cursor);

/* Set a Cursor Option */
int php_mongo_cursor_add_option(mongo_cursor *cursor, char *key, zval *value TSRMLS_DC);

/* Kills a cursor on the server */
void php_mongo_kill_cursor(mongo_connection *con, int64_t cursor_id TSRMLS_DC);

/* Set cursor limit on the cursor object */
void php_mongo_cursor_set_limit(mongo_cursor *cursor, long limit);

/* Forces the bson to zval conversion to use an object for a long */
void php_mongo_cursor_force_long_as_object(mongo_cursor *cursor);

/* Flags the cursor as a command cursor */
void php_mongo_cursor_force_command_cursor(mongo_cursor *cursor);

/* Switch to primary connection */
void php_mongo_cursor_force_primary(mongo_cursor *cursor);

/* Throw a MongoCursorException with the given code and message. Uses the
 * server to fill in information about the connection that caused the exception.
 * Does nothing if an exception has already been thrown. */
zval* php_mongo_cursor_throw(zend_class_entry *exception_ce, mongo_connection *connection, int code TSRMLS_DC, char *format, ...);

/* Returns whether a passed in namespace is a valid one */
int php_mongo_is_valid_namespace(char *ns, int ns_len);
/* Splits a namespace name into the database and collection names, allocated with estrdup. */
void php_mongo_split_namespace(char *ns, char **dbname, char **cname);

/* Reset the cursor to clean up or prepare for another query. Removes cursor
 * from cursor list (and kills it, if necessary).  */
void php_mongo_cursor_reset(mongo_cursor *cursor TSRMLS_DC);

/* Resets cursor and disconnects connection.  Always returns FAILURE (so it can
 * be used by functions returning FAILURE). */
int php_mongo_cursor_failed(mongo_cursor *cursor TSRMLS_DC);
#endif

void mongo_init_MongoCursorInterface(TSRMLS_D);

PHP_METHOD(MongoCursorInterface, batchSize);
PHP_METHOD(MongoCursorInterface, dead);
PHP_METHOD(MongoCursorInterface, info);
PHP_METHOD(MongoCursorInterface, timeout);
PHP_METHOD(MongoCursorInterface, getReadPreference);
PHP_METHOD(MongoCursorInterface, setReadPreference);

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
