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
#include <zend_interfaces.h>
#include <zend_exceptions.h>
#include "mcon/io.h"
#include "mcon/manager.h"
#include "mcon/utils.h"
#include "exceptions/duplicate_key_exception.h"
#include "exceptions/execution_timeout_exception.h"
#include "cursor_shared.h"

#ifdef WIN32
# ifndef int64_t
typedef __int64 int64_t;
# endif
#else
# include <unistd.h>
#endif
#include <math.h>

#include "php_mongo.h"
#include "bson.h"
#include "db.h"
#include "cursor.h"
#include "collection.h"
#include "util/log.h"
#include "log_stream.h"
#include "cursor_shared.h"

/* externs */
extern zend_class_entry *mongo_ce_Id, *mongo_ce_MongoClient, *mongo_ce_DB;
extern zend_class_entry *mongo_ce_Collection, *mongo_ce_Exception;
extern zend_class_entry *mongo_ce_CursorInterface;
extern zend_class_entry *mongo_ce_ConnectionException;
extern zend_class_entry *mongo_ce_CursorException;
extern zend_class_entry *mongo_ce_CursorTimeoutException;
extern zend_class_entry *mongo_ce_DuplicateKeyException;
extern zend_class_entry *mongo_ce_ExecutionTimeoutException;

extern zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

static zend_object_value php_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC);
static int have_error_flags(mongo_cursor *cursor);
static int handle_error(mongo_cursor *cursor TSRMLS_DC);

zend_class_entry *mongo_ce_Cursor = NULL;

/* Queries the database. Returns SUCCESS or FAILURE. */
static int mongo_cursor__do_query(zval *this_ptr, zval *return_value TSRMLS_DC);

/* If the query should be send to the db or not. The rules are:
 * - db commands should only be sent onces (no retries)
 * - normal queries should be sent up to 5 times
 * This uses exponential backoff with a random seed to avoid flooding a
 * struggling db with retries.  */
static int mongo_cursor__should_retry(mongo_cursor *cursor);

#define PREITERATION_SETUP \
	PHP_MONGO_GET_CURSOR(getThis()); \
	\
	if (cursor->started_iterating) { \
		zend_throw_exception(mongo_ce_CursorException, "cannot modify cursor after beginning iteration.", 0 TSRMLS_CC); \
		return; \
	}

/* {{{ MongoCursor->__construct(MongoClient connection, string ns [, array query [, array fields]])
   Constructs a MongoCursor */
PHP_METHOD(MongoCursor, __construct)
{
	zval *zlink = 0, *zquery = 0, *zfields = 0, *empty, *timeout;
	char *ns;
	int   ns_len;
	zval **data;
	mongo_cursor *cursor;
	mongoclient  *link;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os|zz", &zlink, mongo_ce_MongoClient, &ns, &ns_len, &zquery, &zfields) == FAILURE) {
		return;
	}

	if (!php_mongo_is_valid_namespace(ns, ns_len)) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 21 TSRMLS_CC, "An invalid 'ns' argument is given (%s)", ns);
		return;
	}

	MUST_BE_ARRAY_OR_OBJECT(3, zquery);
	MUST_BE_ARRAY_OR_OBJECT(4, zfields);

	cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	link = (mongoclient*)zend_object_store_get_object(zlink TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(link->manager, MongoClient);

	/* if query or fields weren't passed, make them default to an empty array */
	MAKE_STD_ZVAL(empty);
	object_init(empty);

	/* These are both initialized to the same zval, but that's okay because.
	 * There's no way to change them without creating a new cursor */
	if (!zquery || (Z_TYPE_P(zquery) == IS_ARRAY && zend_hash_num_elements(HASH_P(zquery)) == 0)) {
		zquery = empty;
	}
	if (!zfields) {
		zfields = empty;
	}

	/* db connection */
	cursor->zmongoclient = zlink;
	zval_add_ref(&zlink);

	/* change ['x', 'y', 'z'] into {'x' : 1, 'y' : 1, 'z' : 1} */
	if (Z_TYPE_P(zfields) == IS_ARRAY) {
		HashPosition pointer;
		zval *fields;

		MAKE_STD_ZVAL(fields);
		array_init(fields);

		/* fields to return */
		for (
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(zfields), &pointer);
			zend_hash_get_current_data_ex(Z_ARRVAL_P(zfields), (void**) &data, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(Z_ARRVAL_P(zfields), &pointer)
		) {
			int key_type, key_len;
			ulong index;
			char *key;

			key_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(zfields), &key, (uint*)&key_len, &index, NO_DUP, &pointer);

			if (key_type == HASH_KEY_IS_LONG) {
				if (Z_TYPE_PP(data) == IS_STRING) {
					add_assoc_long(fields, Z_STRVAL_PP(data), 1);
				} else {
					zval_ptr_dtor(&empty);
					zval_ptr_dtor(&fields);
					zend_throw_exception(mongo_ce_Exception, "field names must be strings", 8 TSRMLS_CC);
					return;
				}
			} else {
				add_assoc_zval(fields, key, *data);
				zval_add_ref(data);
			}
		}
		cursor->fields = fields;
	} else {
		/* if it's already an object, we don't have to worry */
		cursor->fields = zfields;
		zval_add_ref(&zfields);
	}

	/* ns */
	cursor->ns = estrdup(ns);

	/* query */
	cursor->query = zquery;
	zval_add_ref(&zquery);

	/* reset iteration pointer, just in case */
	php_mongo_cursor_reset(cursor TSRMLS_CC);

	/* Set initial connection to NULL. This allows us to override the cursor
	 * just after construction. THis is useful as a temporary hack until we
	 * have refactored to create cursors with an already configured cursor. See
	 * also the other TODO in this file on this. */
	cursor->connection = NULL;

	cursor->at = 0;
	cursor->num = 0;
	cursor->special = 0;
	cursor->persist = 0;

	timeout = zend_read_static_property(mongo_ce_Cursor, "timeout", strlen("timeout"), NOISY TSRMLS_CC);
	convert_to_long(timeout);
	cursor->timeout = Z_LVAL_P(timeout);

	/* Overwrite the timeout if MongoCursor::$timeout is the default and we
	 * passed in socketTimeoutMS in the connection string */
	if (cursor->timeout == PHP_MONGO_DEFAULT_SOCKET_TIMEOUT && link->servers->options.socketTimeoutMS > 0) {
		cursor->timeout = link->servers->options.socketTimeoutMS;
	}

	/* If the static property "slaveOkay" is set, we need to switch to a
	 * MONGO_RP_SECONDARY_PREFERRED as well, but only if read preferences
	 * aren't already set. */
	if (cursor->read_pref.type == MONGO_RP_PRIMARY) {
		zval *zslaveokay;

		zslaveokay = zend_read_static_property(mongo_ce_Cursor, "slaveOkay", strlen("slaveOkay"), NOISY TSRMLS_CC);
		cursor->read_pref.type = Z_BVAL_P(zslaveokay) ? MONGO_RP_SECONDARY_PREFERRED : MONGO_RP_PRIMARY;
	}

	/* get rid of extra ref */
	zval_ptr_dtor(&empty);
}
/* }}} */

/* {{{ MongoCursor::hasNext
 */
PHP_METHOD(MongoCursor, hasNext)
{
	mongo_buffer buf;
	int size;
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	char *error_message = NULL;
	mongoclient *client;

	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	if (!cursor->started_iterating) {
		MONGO_METHOD(MongoCursor, doQuery, return_value, getThis());
		cursor->started_iterating = 1;
	}

	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

	if ((cursor->limit > 0 && cursor->at >= cursor->limit) || cursor->num == 0) {
		if (cursor->cursor_id != 0) {
			php_mongo_kill_cursor(cursor->connection, cursor->cursor_id TSRMLS_CC);
			cursor->cursor_id = 0;
		}
		RETURN_FALSE;
	}

	if (cursor->at < cursor->num) {
		RETURN_TRUE;
	} else if (cursor->cursor_id == 0) {
		RETURN_FALSE;
	} else if (cursor->connection == NULL) {
		/* if we have a cursor_id, we should have a server */
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 18 TSRMLS_CC, "trying to get more, but cannot find server");
		return;
	}

	/* we have to go and check with the db */
	size = 34 + strlen(cursor->ns);
	CREATE_BUF(buf, size);

	if (FAILURE == php_mongo_write_get_more(&buf, cursor TSRMLS_CC)) {
		efree(buf.start);
		return;
	}
#if MONGO_PHP_STREAMS
	mongo_log_stream_getmore(cursor->connection, cursor TSRMLS_CC);
#endif

	PHP_MONGO_GET_MONGOCLIENT_FROM_CURSOR(cursor);

	if (client->manager->send(cursor->connection, &client->servers->options, buf.start, buf.pos - buf.start, (char **) &error_message) == -1) {
		efree(buf.start);

		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 1 TSRMLS_CC, "%s", error_message);
		free(error_message);
		php_mongo_cursor_failed(cursor TSRMLS_CC);
		return;
	}

	efree(buf.start);

	if (php_mongo_get_reply(cursor TSRMLS_CC) != SUCCESS) {
		/* php_mongo_get_reply() throws exceptions */
		free(error_message);
		php_mongo_cursor_failed(cursor TSRMLS_CC);
		return;
	}

	if (have_error_flags(cursor)) {
		RETURN_TRUE;
	}

	/* if cursor_id != 0, server should stay the same */
	/* sometimes we'll have a cursor_id but there won't be any more results */
	if (cursor->at >= cursor->num) {
		RETVAL_FALSE;
	} else {
		/* but sometimes there will be */
		RETVAL_TRUE;
	}
}
/* }}} */

/* {{{ MongoCursor::getNext
 */
PHP_METHOD(MongoCursor, getNext)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);
	MONGO_CURSOR_CHECK_DEAD;

	MONGO_METHOD(MongoCursor, next, return_value, getThis());
	/* will be null unless there was an error */
	if (EG(exception) || (Z_TYPE_P(return_value) == IS_BOOL && Z_BVAL_P(return_value) == 0)) {
		ZVAL_NULL(return_value);
		return;
	}
	MONGO_CURSOR_CHECK_DEAD;

	if (cursor->current) {
		RETURN_ZVAL(cursor->current, 1, 0);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ MongoCursor::limit
 */
PHP_METHOD(MongoCursor, limit)
{
	long l;
	mongo_cursor *cursor;

	PREITERATION_SETUP;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &l) == FAILURE) {
		return;
	}

	php_mongo_cursor_set_limit(cursor, l);
	RETVAL_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::skip
 */
PHP_METHOD(MongoCursor, skip)
{
	long l;
	mongo_cursor *cursor;

	PREITERATION_SETUP;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &l) == FAILURE) {
		return;
	}

	cursor->skip = l;
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::fields
 */
PHP_METHOD(MongoCursor, fields)
{
	zval *z;
	mongo_cursor *cursor;

	PREITERATION_SETUP;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(1, z);

	zval_ptr_dtor(&cursor->fields);
	cursor->fields = z;
	zval_add_ref(&z);

	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto MongoCursor MongoCursor::maxTimeMS(int ms)
 * Configures a maximum time for the query to run, including fetching results. */
PHP_METHOD(MongoCursor, maxTimeMS)
{
	long time_ms;
	mongo_cursor *cursor;
	zval *value;

	PREITERATION_SETUP;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &time_ms) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_CURSOR(getThis());
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	MAKE_STD_ZVAL(value);
	ZVAL_LONG(value, time_ms);

	if (php_mongo_cursor_add_option(cursor, "$maxTimeMS", value TSRMLS_CC)) {
		RETVAL_ZVAL(getThis(), 1, 0);
	}

	zval_ptr_dtor(&value);
}
/* }}} */

/* {{{ Cursor flags
   Sets or unsets the flag <flag>. With mode = -1, the arguments are parsed.
   Otherwise the mode should contain 0 for unsetting and 1 for setting the flag. */
static inline void set_cursor_flag(INTERNAL_FUNCTION_PARAMETERS, int flag, int mode)
{
	zend_bool z = 1;
	mongo_cursor *cursor;

	PREITERATION_SETUP;

	if (mode == -1) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &z) == FAILURE) {
			return;
		}
	} else {
		z = mode;
	}

	if (z) {
		cursor->opts |= flag;
	} else {
		cursor->opts &= ~flag;
	}

	RETURN_ZVAL(getThis(), 1, 0);
}

/* {{{ MongoCursor::setFlag(int bit [, bool set])
 */
PHP_METHOD(MongoCursor, setFlag)
{
	long      bit;
	zend_bool set = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|b", &bit, &set) == FAILURE) {
		return;
	}
	/* Prevent bit 6 (CURSOR_FLAG_EXHAUST) from being set. This is because the
	 * driver can't handle this at the moment. */
	if (bit == 6) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The CURSOR_FLAG_EXHAUST(6) flag is not supported");
		return;
	}
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1 << bit, set);
}
/* }}} */

/* {{{ MongoCursor::tailable(bool flag)
 */
PHP_METHOD(MongoCursor, tailable)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_TAILABLE, -1);
}
/* }}} */

/* {{{ MongoCursor::slaveOkay(bool flag)
 */
PHP_METHOD(MongoCursor, slaveOkay)
{
	mongo_cursor *cursor;
	zend_bool     slave_okay = 1;

	PREITERATION_SETUP;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &slave_okay) == FAILURE) {
		return;
	}

	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_SLAVE_OKAY, slave_okay);

	/* slaveOkay implicitly sets read preferences.
	 *
	 * With slave_okay being true or absent, the RP is switched to SECONDARY
	 * PREFERRED but only if the current configured RP is PRIMARY - so that
	 * other read preferences are not overwritten. As slaveOkay really only
	 * means "read from any secondary" that does not conflict.
	 *
	 * With slave_okay being false, the RP is switched to PRIMARY. Setting it
	 * to PRIMARY when it already is PRIMARY doesn't hurt. */
	if (slave_okay) {
		if (cursor->read_pref.type == MONGO_RP_PRIMARY) {
			cursor->read_pref.type = MONGO_RP_SECONDARY_PREFERRED;
		}
	} else {
		cursor->read_pref.type = MONGO_RP_PRIMARY;
	}
}
/* }}} */


/* {{{ MongoCursor::immortal(bool flag)
 */
PHP_METHOD(MongoCursor, immortal)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_NO_CURSOR_TO, -1);
}
/* }}} */

/* {{{ MongoCursor::awaitData(bool flag)
 */
PHP_METHOD(MongoCursor, awaitData)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_AWAIT_DATA, -1);
}
/* }}} */

/* {{{ MongoCursor::partial(bool flag)
 */
PHP_METHOD(MongoCursor, partial)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_PARTIAL, -1);
}
/* }}} */
/* }}} */


/* {{{ MongoCursor::timeout
 */
PHP_METHOD(MongoCursor, timeout) {
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

PHP_METHOD(MongoCursor, getReadPreference)
{
	mongo_cursor *cursor;
	PHP_MONGO_GET_CURSOR(getThis());

	array_init(return_value);
	add_assoc_string(return_value, "type", mongo_read_preference_type_to_name(cursor->read_pref.type), 1);
	php_mongo_add_tagsets(return_value, &cursor->read_pref);
}

/* {{{ MongoCursor::setReadPreference(string read_preference [, array tags ])
   Sets a read preference to be used for all read queries.*/
PHP_METHOD(MongoCursor, setReadPreference)
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

/* {{{ MongoCursor::addOption
 */
PHP_METHOD(MongoCursor, addOption)
{
	char *key;
	int key_len;
	zval *value;
	mongo_cursor *cursor;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &key, &key_len, &value) == FAILURE) {
		return;
	}
	PHP_MONGO_GET_CURSOR(getThis());
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	if (php_mongo_cursor_add_option(cursor, key, value TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}
}
/* }}} */

/* {{{ MongoCursor::snapshot
 */
PHP_METHOD(MongoCursor, snapshot)
{
	zval *yes;
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	MAKE_STD_ZVAL(yes);
	ZVAL_TRUE(yes);

	if (php_mongo_cursor_add_option(cursor, "$snapshot", yes TSRMLS_CC)) {
		RETVAL_ZVAL(getThis(), 1, 0);
	}

	zval_ptr_dtor(&yes);
}
/* }}} */


/* {{{ MongoCursor->sort(array fields)
 */
PHP_METHOD(MongoCursor, sort)
{
	zval *fields;
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &fields) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(1, fields);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	if (php_mongo_cursor_add_option(cursor, "$orderby", fields TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}
}
/* }}} */

/* {{{ proto MongoCursor MongoCursor::hint(mixed index)
   Hint the index, by name or fields, to use for the query. */
PHP_METHOD(MongoCursor, hint)
{
	zval *index;
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &index) == FAILURE) {
		return;
	}
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	if (php_mongo_cursor_add_option(cursor, "$hint", index TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}
}
/* }}} */

/* {{{ MongoCursor->explain
 */
PHP_METHOD(MongoCursor, explain)
{
	int temp_limit;
	zval *yes;
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	php_mongo_cursor_reset(cursor TSRMLS_CC);

	/* make explain use a hard limit */
	temp_limit = cursor->limit;
	if (cursor->limit > 0) {
		cursor->limit *= -1;
	}

	MAKE_STD_ZVAL(yes);
	ZVAL_TRUE(yes);

	if (!php_mongo_cursor_add_option(cursor, "$explain", yes TSRMLS_CC)) {
		zval_ptr_dtor(&yes);
		return;
	}

	zval_ptr_dtor(&yes);

	MONGO_METHOD(MongoCursor, getNext, return_value, getThis());

	/* reset cursor to original state */
	cursor->limit = temp_limit;
	zend_hash_del(HASH_P(cursor->query), "$explain", strlen("$explain") + 1);

	php_mongo_cursor_reset(cursor TSRMLS_CC);
}
/* }}} */


/* {{{ MongoCursor->doQuery
 */
PHP_METHOD(MongoCursor, doQuery)
{
	mongo_cursor *cursor;

	PHP_MONGO_GET_CURSOR(getThis());
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	do {
		php_mongo_cursor_reset(cursor TSRMLS_CC);
		if (mongo_cursor__do_query(getThis(), return_value TSRMLS_CC) == SUCCESS || EG(exception)) {
			return;
		}
	} while (mongo_cursor__should_retry(cursor));

	if (strcmp(".$cmd", cursor->ns+(strlen(cursor->ns)-5)) == 0) {
		php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 19 TSRMLS_CC, "couldn't send command");
		return;
	}

	php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 19 TSRMLS_CC, "max number of retries exhausted, couldn't send query");
}
/* }}} */

/* Cursor helpers */
int mongo_cursor_mark_dead(void *callback_data)
{
	mongo_cursor *cursor = (mongo_cursor*) callback_data;

	cursor->dead = 1;
	cursor->connection = NULL;

	return 1;
}

/* Adds the $readPreference option to the query objects */
void mongo_apply_mongos_rp(mongo_cursor *cursor)
{
	zval *rp, *tags;
	char *type;

	/* Older mongos don't like $readPreference, so don't apply it
	 * when we want the default behaviour anyway */
	if (cursor->read_pref.type == MONGO_RP_PRIMARY) {
		return;
	}
	if (cursor->read_pref.type == MONGO_RP_SECONDARY_PREFERRED) {
		/* If there aren't any tags, don't add $readPreference, the slaveOkay
		 * flag is enough This gives us improved compatability with older
		 * mongos */
		if (cursor->read_pref.tagset_count == 0) {
			return;
		}
	}

	type = mongo_read_preference_type_to_name(cursor->read_pref.type);
	MAKE_STD_ZVAL(rp);
	array_init(rp);
	add_assoc_string(rp, "mode", type, 1);

	tags = php_mongo_make_tagsets(&cursor->read_pref);
	if (tags) {
		add_assoc_zval(rp, "tags", tags);
	}

	php_mongo_make_special(cursor);
	add_assoc_zval(cursor->query, "$readPreference", rp);
}

static int mongo_cursor__do_query(zval *this_ptr, zval *return_value TSRMLS_DC)
{
	mongo_cursor *cursor;
	mongo_buffer buf;
	char *error_message;
	mongoclient *link;
	mongo_read_preference rp;

	cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (!cursor) {
		zend_throw_exception(mongo_ce_Exception, "The MongoCursor object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
		return FAILURE;
	}

	/* db connection zmongoclient */
	link = (mongoclient*)zend_object_store_get_object(cursor->zmongoclient TSRMLS_CC);
	if (!link->servers) {
		zend_throw_exception(mongo_ce_Exception, "The Mongo object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
		return FAILURE;
	}

	/* If we had a connection we need to remove it from the callback map before
	 * we assign it another connection. */
	if (cursor->connection) {
		mongo_deregister_callback_from_connection(cursor->connection, cursor);
	}

	/* Sets the wire protocol flag to allow reading from a secondary. The read
	 * preference spec states: "slaveOk remains as a bit in the wire protocol
	 * and drivers will set this bit to 1 for all reads except with PRIMARY
	 * read preference." */
	cursor->opts = cursor->opts | (cursor->read_pref.type != MONGO_RP_PRIMARY ? CURSOR_FLAG_SLAVE_OKAY : 0);

	/* store the link's read preference to backup, and overwrite with the
	 * cursors's read preferences */
	mongo_read_preference_copy(&link->servers->read_pref, &rp);
	mongo_read_preference_replace(&cursor->read_pref, &link->servers->read_pref);

	/* TODO: We have to assume to use a read connection here, but it should
	 * really be refactored so that we can create a cursor with the correct
	 * read/write setup already, instead of having to force a new mode later
	 * (like we do for commands right now through
	 * php_mongo_cursor_force_primary).  See also MongoDB::command and
	 * append_getlasterror, where this has to be done too. */
	cursor->connection = mongo_get_read_write_connection_with_callback(
		link->manager,
		link->servers,
		cursor->cursor_options & MONGO_CURSOR_OPT_FORCE_PRIMARY ? MONGO_CON_FLAG_WRITE : MONGO_CON_FLAG_READ,
		cursor,
		mongo_cursor_mark_dead,
		(char**) &error_message
	);

	/* restore read preferences from backup */
	mongo_read_preference_replace(&rp, &link->servers->read_pref);
	mongo_read_preference_dtor(&rp);

	/* Throw exception in case we have no connection */
	if (cursor->connection == NULL) {
		if (error_message) {
			zend_throw_exception(mongo_ce_ConnectionException, error_message, 71 TSRMLS_CC);
			free(error_message);
		} else {
			zend_throw_exception(mongo_ce_ConnectionException, "Could not retrieve connection", 72 TSRMLS_CC);
		}
		return FAILURE;
	}

	/* Apply read preference query option, but only if we have a MongoS
	 * connection */
	if (cursor->connection->connection_type == MONGO_NODE_MONGOS) {
		mongo_apply_mongos_rp(cursor);
	}

	/* Create query buffer */
	CREATE_BUF(buf, INITIAL_BUF_SIZE);
	if (php_mongo_write_query(&buf, cursor, cursor->connection->max_bson_size, cursor->connection->max_message_size TSRMLS_CC) == FAILURE) {
		efree(buf.start);
		return FAILURE;
	}
#if MONGO_PHP_STREAMS
	mongo_log_stream_query(cursor->connection, cursor TSRMLS_CC);
#endif

	if (link->manager->send(cursor->connection, &link->servers->options, buf.start, buf.pos - buf.start, (char **) &error_message) == -1) {
		if (error_message) {
			php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 14 TSRMLS_CC, "couldn't send query: %s", error_message);
			free(error_message);
		} else {
			php_mongo_cursor_throw(mongo_ce_CursorException, cursor->connection, 14 TSRMLS_CC, "couldn't send query");
		}
		efree(buf.start);
		
		return php_mongo_cursor_failed(cursor TSRMLS_CC);
	}

	efree(buf.start);

	if (php_mongo_get_reply(cursor TSRMLS_CC) == FAILURE) {
		/* php_mongo_get_reply() throws exceptions */
		return php_mongo_cursor_failed(cursor TSRMLS_CC);
	}

	return SUCCESS;
}
/* }}} */

/* ITERATOR FUNCTIONS */

/* {{{ MongoCursor->current
 */
PHP_METHOD(MongoCursor, current)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);
	MONGO_CURSOR_CHECK_DEAD;

	if (cursor->current) {
		RETURN_ZVAL(cursor->current, 1, 0);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ MongoCursor->key
 */
PHP_METHOD(MongoCursor, key)
{
	zval **id;
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	if (!cursor->current) {
		RETURN_NULL();
	}

	if (cursor->current && Z_TYPE_P(cursor->current) == IS_ARRAY && zend_hash_find(HASH_P(cursor->current), "_id", 4, (void**)&id) == SUCCESS) {
		if (Z_TYPE_PP(id) == IS_OBJECT) {
			zend_std_cast_object_tostring(*id, return_value, IS_STRING TSRMLS_CC);
		} else {
			RETVAL_ZVAL(*id, 1, 0);
			convert_to_string(return_value);
		}
	} else {
		RETURN_LONG(cursor->at - 1);
	}
}
/* }}} */

static int mongo_cursor__should_retry(mongo_cursor *cursor)
{
	int microseconds = 50000, slots = 0, wait_us = 0;

	/* never retry commands */
	if (cursor->retry >= 5 || strcmp(".$cmd", cursor->ns+(strlen(cursor->ns)-5)) == 0) {
		return 0;
	}

	slots = (int)pow(2.0, cursor->retry++);
	wait_us = (rand() % slots) * microseconds;

#ifdef WIN32
	/* Windows sleep takes milliseconds */
	Sleep(wait_us/1000);
#else
	{
		/* usleep is deprecated */
		struct timespec wait;

		wait.tv_sec = wait_us / 1000000;
		wait.tv_nsec = (wait_us % 1000000) * 1000;

		nanosleep(&wait, 0);
	}
#endif

	return 1;
}

/* Returns 1 when an error was found and it returns 0 if no error
 * situation has ocurred on the cursor */
static int have_error_flags(mongo_cursor *cursor)
{
	if (cursor->flag & MONGO_OP_REPLY_ERROR_FLAGS) {
		return 1;
	}

	return 0;
}

/* Returns 1 when an error was found and *handled*, and it returns 0 if no
 * error situation has ocurred on the cursor. If the error is handled, then an
 * exception has been thrown as well. */
static int handle_error(mongo_cursor *cursor TSRMLS_DC)
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
		zval_ptr_dtor(&cursor->current);
		cursor->current = 0;

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

/* {{{ MongoCursor->next
 */
PHP_METHOD(MongoCursor, next)
{
	zval has_next;
	mongo_cursor *cursor;

	PHP_MONGO_GET_CURSOR(getThis());
	MONGO_CURSOR_CHECK_DEAD;

	if (!cursor->started_iterating) {
		MONGO_METHOD(MongoCursor, doQuery, return_value, getThis());
		if (EG(exception)) {
			return;
		}
		cursor->started_iterating = 1;
	}

	/* destroy old current */
	if (cursor->current) {
		zval_ptr_dtor(&cursor->current);
		cursor->current = 0;
	}

	/* check for results */
	MONGO_METHOD(MongoCursor, hasNext, &has_next, getThis());
	if (EG(exception)) {
		return;
	}

	if (!Z_BVAL(has_next)) {
		/* we're out of results */
		/* Might throw an exception */
		handle_error(cursor TSRMLS_CC);
		RETURN_NULL();
	}

	/* we got more results */
	if (cursor->at < cursor->num) {
		mongo_bson_conversion_options options = MONGO_BSON_CONVERSION_OPTIONS_INIT;

		if (cursor->cursor_options & MONGO_CURSOR_OPT_CMD_CURSOR) {
			options.level = 0;
			options.flag_cmd_cursor_as_int64 = 1;
		}

		MAKE_STD_ZVAL(cursor->current);
		array_init(cursor->current);
		cursor->buf.pos = bson_to_zval(
			(char*)cursor->buf.pos,
			Z_ARRVAL_P(cursor->current),
			&options
			TSRMLS_CC
		);

		if (EG(exception)) {
			zval_ptr_dtor(&cursor->current);
			cursor->current = 0;
			return;
		}

		/* increment cursor position */
		cursor->at++;

		/* Might throw an exception */
		if (handle_error(cursor TSRMLS_CC)) {
			RETURN_NULL();
		}
	}

	RETURN_NULL();
}
/* }}} */

/* {{{ MongoCursor->rewind
 */
PHP_METHOD(MongoCursor, rewind)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	php_mongo_cursor_reset(cursor TSRMLS_CC);
	MONGO_METHOD(MongoCursor, next, return_value, getThis());
}
/* }}} */

/* {{{ MongoCursor->valid
 */
PHP_METHOD(MongoCursor, valid)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	RETURN_BOOL(cursor->current);
}
/* }}} */

/* {{{ MongoCursor->reset
 */
PHP_METHOD(MongoCursor, reset)
{
	mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

	php_mongo_cursor_reset(cursor TSRMLS_CC);
}
/* }}} */

PHP_METHOD(MongoCursor, count)
{
	zend_bool all = 0;
	zval *response, *cmd, **n;
	mongo_cursor *cursor;
	mongoclient *link;
	char *cname, *dbname;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &all) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_CURSOR(getThis());
	PHP_MONGO_GET_LINK(cursor->zmongoclient);

	php_mongo_split_namespace(cursor->ns, &dbname, &cname);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_string(cmd, "count", cname, 0);

	if (cursor->query) {
		zval **inner_query = 0;

		if (!cursor->special) {
			add_assoc_zval(cmd, "query", cursor->query);
			zval_add_ref(&cursor->query);
		} else if (zend_hash_find(HASH_P(cursor->query), "$query", strlen("$query") + 1, (void**)&inner_query) == SUCCESS) {
			add_assoc_zval(cmd, "query", *inner_query);
			zval_add_ref(inner_query);
		}
	}

	if (all) {
		add_assoc_long(cmd, "limit", cursor->limit);
		add_assoc_long(cmd, "skip", cursor->skip);
	}

	response = php_mongo_runcommand(cursor->zmongoclient, &cursor->read_pref, dbname, strlen(dbname), cmd, NULL, 0, NULL TSRMLS_CC);
	zval_ptr_dtor(&cmd);
	efree(dbname);

	if (EG(exception) || Z_TYPE_P(response) != IS_ARRAY) {
		zval_ptr_dtor(&response);
		return;
	}

	/* FIXME: Refactor into php_mongo_count() */
	if (zend_hash_find(HASH_P(response), "n", 2, (void**)&n) == SUCCESS) {
		convert_to_long(*n);
		RETVAL_ZVAL(*n, 1, 0);
		zval_ptr_dtor(&response);
	} else {
		zval **errmsg;

		/* The command failed, try to find an error message */
		if (zend_hash_find(HASH_P(response), "errmsg", strlen("errmsg") + 1 , (void**)&errmsg) == SUCCESS) {
			zend_throw_exception_ex(mongo_ce_Exception, 20 TSRMLS_CC, "Cannot run command count(): %s", Z_STRVAL_PP(errmsg));
		} else {
			zend_throw_exception(mongo_ce_Exception, "Cannot run command count()", 20 TSRMLS_CC);
		}
		zval_ptr_dtor(&response);
	}
}

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(0, connection, MongoClient, 0)
	ZEND_ARG_INFO(0, database_and_collection_name)
	ZEND_ARG_INFO(0, query)
	ZEND_ARG_INFO(0, array_of_fields_OR_object)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_batchsize, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_skip, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_fields, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, fields)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_maxtimems, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, ms)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_add_option, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, fields)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_hint, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, keyPattern)
ZEND_END_ARG_INFO()

/* {{{ Cursor flags */
MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_set_flag, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, bit)
	ZEND_ARG_INFO(0, set)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_tailable, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, tail)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_slave_okay, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, okay)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_immortal, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, liveForever)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_await_data, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, wait)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_partial, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, okay)
ZEND_END_ARG_INFO()
/* }}} */

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_count, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, foundOnly)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_setReadPreference, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, read_preference)
	ZEND_ARG_ARRAY_INFO(0, tags, 0)
ZEND_END_ARG_INFO()

static zend_function_entry MongoCursor_methods[] = {
	PHP_ME(MongoCursor, __construct, arginfo___construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, hasNext, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, getNext, arginfo_no_parameters, ZEND_ACC_PUBLIC)

	/* options */
	PHP_ME(MongoCursor, limit, arginfo_limit, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursorInterface, batchSize, arginfo_batchsize, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, skip, arginfo_skip, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, fields, arginfo_fields, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, maxTimeMS, arginfo_maxtimems, ZEND_ACC_PUBLIC)

	/* meta options */
	PHP_ME(MongoCursor, addOption, arginfo_add_option, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, snapshot, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, sort, arginfo_sort, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, hint, arginfo_hint, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, explain, arginfo_no_parameters, ZEND_ACC_PUBLIC)

	/* flags */
	PHP_ME(MongoCursor, setFlag, arginfo_set_flag, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, slaveOkay, arginfo_slave_okay, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoCursor, tailable, arginfo_tailable, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, immortal, arginfo_immortal, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, awaitData, arginfo_await_data, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, partial, arginfo_partial, ZEND_ACC_PUBLIC)

	/* read preferences */
	PHP_ME(MongoCursor, getReadPreference, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, setReadPreference, arginfo_setReadPreference, ZEND_ACC_PUBLIC)

	/* query */
	PHP_ME(MongoCursor, timeout, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, doQuery, arginfo_no_parameters, ZEND_ACC_PROTECTED|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoCursorInterface, info, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursorInterface, dead, arginfo_no_parameters, ZEND_ACC_PUBLIC)

	/* iterator funcs */
	PHP_ME(MongoCursor, current, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, key, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, next, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, rewind, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, valid, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursor, reset, arginfo_no_parameters, ZEND_ACC_PUBLIC)

	/* stand-alones */
	PHP_ME(MongoCursor, count, arginfo_count, ZEND_ACC_PUBLIC)

	PHP_FE_END
};

static zend_object_value php_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC) {
	PHP_MONGO_OBJ_NEW(mongo_cursor);
}

void mongo_init_MongoCursor(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoCursor", MongoCursor_methods);
	ce.create_object = php_mongo_cursor_new;
	mongo_ce_Cursor = zend_register_internal_class(&ce TSRMLS_CC);
	zend_class_implements(mongo_ce_Cursor TSRMLS_CC, 1, mongo_ce_CursorInterface);

	zend_declare_property_bool(mongo_ce_Cursor, "slaveOkay", strlen("slaveOkay"), 0, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_long(mongo_ce_Cursor, "timeout", strlen("timeout"), PHP_MONGO_DEFAULT_SOCKET_TIMEOUT, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
