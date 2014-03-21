/**
 *  Copyright 2013-2014 MongoDB, Inc.
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

#include "php_mongo.h"
#include "bson.h"
#include "cursor.h"
#include "cursor_shared.h"
#include "command_cursor.h"
#include "mcon/manager.h"
#include "db.h"
#include "log_stream.h"

/* externs */
extern zend_class_entry *mongo_ce_MongoClient;
extern zend_class_entry *mongo_ce_CursorInterface;
extern zend_class_entry *mongo_ce_Exception, *mongo_ce_CursorException;
extern zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

zend_class_entry *mongo_ce_CommandCursor = NULL;

#define MONGO_DEFAULT_COMMAND_BATCH_SIZE 101

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_OBJ_INFO(0, connection, MongoClient, 0)
	ZEND_ARG_INFO(0, server_hash)
	ZEND_ARG_ARRAY_INFO(0, cursordoc, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_batchsize, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

void mongo_command_cursor_init(mongo_command_cursor *cmd_cursor, zval *zlink, mongo_connection *connection, zval *first_batch, char *ns, int64_t id TSRMLS_DC)
{
	/* MongoClient instance */
	cmd_cursor->zmongoclient = zlink;
	zval_add_ref(&zlink);

	/* reset iteration pointer and flags */
	php_mongo_cursor_reset((mongo_cursor*)cmd_cursor TSRMLS_CC);
	cmd_cursor->special = 0;

	cmd_cursor->query = NULL;
	cmd_cursor->connection = connection;
	cmd_cursor->ns = estrdup(ns);
	cmd_cursor->cursor_id = id;
	cmd_cursor->first_batch = first_batch;
	Z_ADDREF_P(first_batch);
	cmd_cursor->first_batch_at = 0;
	cmd_cursor->first_batch_num = HASH_OF(cmd_cursor->first_batch)->nNumOfElements;

	/* flags */
	php_mongo_cursor_force_command_cursor(cmd_cursor);
}

int mongo_extract_cursor_ns_and_id(zval *cursordoc, char **ns, int64_t *id)
{
	zval **zid, **zns;

	if (zend_hash_find(Z_ARRVAL_P(cursordoc), "id", sizeof("id"), (void **)&zid) == FAILURE) {
		return FAILURE;
	}
	convert_to_long_ex(zid);

	if (zend_hash_find(Z_ARRVAL_P(cursordoc), "ns", sizeof("ns"), (void **)&zns) == FAILURE) {
		return FAILURE;
	}
	convert_to_string_ex(zns);

	*id = Z_LVAL_PP(zid);
	*ns = Z_STRVAL_PP(zns);

	return SUCCESS;
}
void mongo_command_cursor_init_from_document(zval *zlink, mongo_command_cursor *cmd_cursor, char *hash, zval *cursordoc)
{
	mongo_connection *connection;
	mongoclient *link;
	zval **first_batch;
	char *ns;
	int64_t id;

	link = (mongoclient*)zend_object_store_get_object(zlink TSRMLS_CC);
	if (!link) {
		zend_throw_exception(mongo_ce_Exception, "The MongoCollection object has not been correctly initialized by its constructor", 17 TSRMLS_CC);
		return;
	}

	connection = mongo_manager_connection_find_by_hash(link->manager, hash);
	if (!connection) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 21 TSRMLS_CC, "Cannot find connection associated with: '%s'", hash);
		return;
	}

	if (zend_hash_find(Z_ARRVAL_P(cursordoc), "firstBatch", sizeof("firstBatch"), (void **)&first_batch) == FAILURE) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 21 TSRMLS_CC, "Cursor description does not look real, missing firstBatch");
		return;
	}
	if (mongo_extract_cursor_ns_and_id(cursordoc, &ns, &id) == FAILURE) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 21 TSRMLS_CC, "Cursor description does not look real");
		return;
	}

	mongo_command_cursor_init(cmd_cursor, zlink, connection, *first_batch, ns, id TSRMLS_CC);
}

/* {{{ MongoCommandCursor::__construct(MongoClient connection, string hash, array cursorsdoc)
   Constructs a MongoCommandCursor */
PHP_METHOD(MongoCommandCursor, __construct)
{
	zval *zlink = NULL, *cursordoc = NULL;
	char *hash;
	int   hash_len;
	mongo_command_cursor *cmd_cursor;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Osa", &zlink, mongo_ce_MongoClient, &hash, &hash_len, &cursordoc) == FAILURE) {
		return;
	}

	cmd_cursor = (mongo_command_cursor*) zend_object_store_get_object(getThis() TSRMLS_CC);
	mongo_command_cursor_init_from_document(zlink, cmd_cursor, hash, cursordoc TSRMLS_CC);
}
/* }}} */

/* {{{ array MongoCommandCursor::rewind()
   Resets the command cursor, executes the associated query and prepares the iterator. Returns the raw command document */
PHP_METHOD(MongoCommandCursor, rewind)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	cmd_cursor->started_iterating = 1;

	RETURN_TRUE;
}
/* }}} */

static int fetch_next_batch(mongo_cursor *cursor TSRMLS_DC)
{
	mongo_buffer buf;
	int          size;
	char        *error_message;
	mongoclient *client;

	size = 34 + strlen(cursor->ns);
	CREATE_BUF(buf, size);

	if (FAILURE == php_mongo_write_get_more(&buf, cursor TSRMLS_CC)) {
		efree(buf.start);
		return 0;
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
		return 0;
	}

	efree(buf.start);

	if (php_mongo_get_reply(cursor TSRMLS_CC) != SUCCESS) {
		free(error_message);
		php_mongo_cursor_failed(cursor TSRMLS_CC);
		return 0;
	}

	return 1;
}


/* {{{ bool MongoCommandCursor::valid()
   Returns whether the current iterator position is valid and fetches the key/value associated with the position. */
PHP_METHOD(MongoCommandCursor, valid)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zval **current;

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	if (!cmd_cursor->started_iterating) {
		RETURN_FALSE;
	}

	/* Free the previous current item */
	if (cmd_cursor->current) {
		zval_ptr_dtor(&cmd_cursor->current);
		cmd_cursor->current = NULL;
	}

	/* Do processing of the first batch */
	if (cmd_cursor->first_batch) {
		if (cmd_cursor->first_batch_at < cmd_cursor->first_batch_num) {
			if (zend_hash_index_find(HASH_OF(cmd_cursor->first_batch), cmd_cursor->first_batch_at, (void**) &current) == SUCCESS) {
				cmd_cursor->current = *current;
				Z_ADDREF_PP(current);
				RETURN_TRUE;
			}
		} else {
			/* We're out of bounds for the first batch */
			zval_ptr_dtor(&cmd_cursor->first_batch);
			cmd_cursor->first_batch = NULL;

			/* Now see whether we have a cursor ID, and if we do, call getmore */
			if (cmd_cursor->cursor_id != 0) {
				if (!fetch_next_batch(cmd_cursor TSRMLS_CC)) {
					RETURN_FALSE;
				}
			}
		}
	}

	if (cmd_cursor->at == cmd_cursor->num && cmd_cursor->cursor_id != 0) {
		if (!fetch_next_batch(cmd_cursor TSRMLS_CC)) {
			cmd_cursor->cursor_id = 0;
			RETURN_FALSE;
		}
	}

	/* Check for the subsequent batches, this is like a normal cursor now */
	if (cmd_cursor->at < cmd_cursor->num) {
		MAKE_STD_ZVAL(cmd_cursor->current);
		array_init(cmd_cursor->current);
		cmd_cursor->buf.pos = bson_to_zval(
			(char*)cmd_cursor->buf.pos,
			Z_ARRVAL_P(cmd_cursor->current),
			NULL
			TSRMLS_CC
		);

		if (EG(exception)) {
			zval_ptr_dtor(&cmd_cursor->current);
			cmd_cursor->current = NULL;
			RETURN_FALSE;
		}

		RETURN_TRUE;
	}

	cmd_cursor->dead = 1;
	RETURN_FALSE;
}
/* }}} */

/* {{{ void MongoCommandCursor::next()
   Advances the interal cursor position. */
PHP_METHOD(MongoCommandCursor, next)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	if (!cmd_cursor->started_iterating) {
		zend_throw_exception(mongo_ce_CursorException, "can only iterate after the command has been run.", 0 TSRMLS_CC); \
	}

	if (cmd_cursor->first_batch) {
		cmd_cursor->first_batch_at++;
		return;
	}

	cmd_cursor->at++;
}
/* }}} */

/* {{{ mixed MongoCommandCursor::current()
   Returns the data associated with the current cursor position. */
PHP_METHOD(MongoCommandCursor, current)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	if (cmd_cursor->current) {
		RETVAL_ZVAL(cmd_cursor->current, 1, 0);
	}
}
/* }}} */

/* {{{ mixed MongoCommandCursor::key()
   Returns the key associated with the current cursor position. */
PHP_METHOD(MongoCommandCursor, key)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zval **id;

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCursor);

	if (!cmd_cursor->current) {
		RETURN_NULL();
	}

	if (cmd_cursor->current && Z_TYPE_P(cmd_cursor->current) == IS_ARRAY && zend_hash_find(HASH_P(cmd_cursor->current), "_id", 4, (void**)&id) == SUCCESS) {
		if (Z_TYPE_PP(id) == IS_OBJECT) {
			zend_std_cast_object_tostring(*id, return_value, IS_STRING TSRMLS_CC);
		} else {
			RETVAL_ZVAL(*id, 1, 0);
			convert_to_string(return_value);
		}
	} else {
		if (cmd_cursor->first_batch) {
			RETURN_LONG(cmd_cursor->first_batch_at - 1);
		} else {
			RETURN_LONG(cmd_cursor->first_batch_num + cmd_cursor->at - 1);
		}
	}
}
/* }}} */

static zend_function_entry MongoCommandCursor_methods[] = {
	PHP_ME(MongoCommandCursor, __construct, arginfo___construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)

	/* options, code is reused from MongoCursor */
	PHP_ME(MongoCursorInterface, batchSize, arginfo_batchsize, ZEND_ACC_PUBLIC)

	/* query, code is shared through MongoCursorInterface */
	PHP_ME(MongoCursorInterface, info, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCursorInterface, dead, arginfo_no_parameters, ZEND_ACC_PUBLIC)

	/* iterator funcs */
	PHP_ME(MongoCommandCursor, current, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCommandCursor, key, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCommandCursor, next, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCommandCursor, rewind, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCommandCursor, valid, arginfo_no_parameters, ZEND_ACC_PUBLIC)

	PHP_FE_END
};

/* The PHP_MONGO_OBJ_NEW macro needs a specifically named function for free, but
 * as mongo_cursor is both used for normal and command cursors, we need this extra
 * intermediary function */
void php_mongo_command_cursor_free(void *object TSRMLS_DC)
{
	php_mongo_cursor_free(object TSRMLS_CC);
}

static zend_object_value php_mongo_command_cursor_new(zend_class_entry *class_type TSRMLS_DC)
{
	PHP_MONGO_OBJ_NEW(mongo_command_cursor);
}

void mongo_init_MongoCommandCursor(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoCommandCursor", MongoCommandCursor_methods);
	ce.create_object = php_mongo_command_cursor_new;
	mongo_ce_CommandCursor = zend_register_internal_class(&ce TSRMLS_CC);
	zend_class_implements(mongo_ce_CommandCursor TSRMLS_CC, 1, mongo_ce_CursorInterface);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
