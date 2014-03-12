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

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(0, connection, MongoClient, 0)
	ZEND_ARG_INFO(0, database_and_collection_name)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_batchsize, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

void mongo_command_cursor_init(mongo_command_cursor *cmd_cursor, char *ns, zval *zlink, zval *zcommand TSRMLS_DC)
{
	mongoclient *link;

	link = (mongoclient*) zend_object_store_get_object(zlink TSRMLS_CC);

	/* ns */
	cmd_cursor->ns = estrdup(ns);

	/* db connection */
	cmd_cursor->zmongoclient = zlink;
	zval_add_ref(&zlink);

	/* query */
	cmd_cursor->query = zcommand;
	zval_add_ref(&zcommand);

	/* reset iteration pointer and flags */
	php_mongo_cursor_reset((mongo_cursor*)cmd_cursor TSRMLS_CC);
	cmd_cursor->special = 0;

	/* flags */
	php_mongo_cursor_force_command_cursor(cmd_cursor);

	/* Pick up read preferences from link as initial setting */
	mongo_read_preference_replace(&link->servers->read_pref, &cmd_cursor->read_pref);
}

/* {{{ MongoCommandCursor::__construct(MongoClient connection, string ns, array query)
   Constructs a MongoCommandCursor */
PHP_METHOD(MongoCommandCursor, __construct)
{
	zval *zlink = NULL, *zcommand = NULL;
	char *ns;
	int   ns_len;
	mongo_command_cursor *cmd_cursor;
	mongoclient *link;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Osa", &zlink, mongo_ce_MongoClient, &ns, &ns_len, &zcommand) == FAILURE) {
		return;
	}

	if (!php_mongo_is_valid_namespace(ns, ns_len)) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 21 TSRMLS_CC, "An invalid 'ns' argument is given (%s)", ns);
		return;
	}

	cmd_cursor = (mongo_command_cursor*) zend_object_store_get_object(getThis() TSRMLS_CC);
	link = (mongoclient*) zend_object_store_get_object(zlink TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(link->manager, MongoClient);

	mongo_command_cursor_init(cmd_cursor, ns, zlink, zcommand TSRMLS_CC);
}
/* }}} */

static int get_batch_size_from_command(zval *command TSRMLS_DC)
{
	int    size = MONGO_DEFAULT_COMMAND_BATCH_SIZE;
	zval **zcursor, **zbatchsize;

	if (Z_TYPE_P(command) != IS_ARRAY) {
		goto end;
	}

	if (zend_hash_find(HASH_P(command), "cursor", 7, (void**) &zcursor) == FAILURE) {
		goto end;
	}

	if (Z_TYPE_PP(zcursor) != IS_ARRAY) {
		goto end;
	}

	if (zend_hash_find(HASH_PP(zcursor), "batchSize", 10, (void**) &zbatchsize) == FAILURE) {
		goto end;
	}

	convert_to_long_ex(zbatchsize);
	size = Z_LVAL_PP(zbatchsize);

end:
	return size;
}

static void enforce_batch_size_on_command(zval *command, int size TSRMLS_DC)
{
	zval **zcursor, **zbatchsize;

	if (Z_TYPE_P(command) != IS_ARRAY) {
		/* Technically we can not hit this, as the command should be an
		 * array, but this is just here as a precaution. */
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 32 TSRMLS_CC, "The cursor command structure is not an array");
		return;
	}

	if (zend_hash_find(HASH_P(command), "cursor", 7, (void**) &zcursor) == FAILURE) {
		zval *tmp_cursor;

		MAKE_STD_ZVAL(tmp_cursor);
		array_init(tmp_cursor);
		zcursor = &tmp_cursor;

		add_assoc_zval(command, "cursor", tmp_cursor);
	}

	if (Z_TYPE_PP(zcursor) != IS_ARRAY) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 32 TSRMLS_CC, "The cursor command's 'cursor' element is not an array");
		return;
	}

	if (zend_hash_find(HASH_PP(zcursor), "batchSize", 10, (void**) &zbatchsize) == FAILURE) {
		zval *tmp_batchsize;

		MAKE_STD_ZVAL(tmp_batchsize);
		ZVAL_LONG(tmp_batchsize, size);
		zbatchsize = &tmp_batchsize;

		add_assoc_zval(*zcursor, "batchSize", tmp_batchsize);
	}
}

/* {{{ array MongoCommandCursor::rewind()
   Resets the command cursor, executes the associated query and prepares the iterator. Returns the raw command document */
PHP_METHOD(MongoCommandCursor, rewind)
{
	char *dbname;
	zval *result;
	int64_t cursor_id;
	zval *exception;
	zval *first_batch;
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	php_mongo_cursor_reset((mongo_cursor*)cmd_cursor TSRMLS_CC);

	/* reads the batchsize through the command, or uses 101 when nothing is set */
	cmd_cursor->batch_size = get_batch_size_from_command(cmd_cursor->query TSRMLS_CC);
	enforce_batch_size_on_command(cmd_cursor->query, cmd_cursor->batch_size TSRMLS_CC);
	if (EG(exception)) {
		return;
	}

	/* do query */
	php_mongo_split_namespace(cmd_cursor->ns, &dbname, NULL);
	result = php_mongo_runcommand(cmd_cursor->zmongoclient, &cmd_cursor->read_pref, dbname, strlen(dbname), cmd_cursor->query, NULL, 1, &cmd_cursor->connection TSRMLS_CC);
	efree(dbname);

	if (php_mongo_trigger_error_on_command_failure(cmd_cursor->connection, result TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&result);
		return;
	}

	/* We need to parse the initial result, and see whether everything worked */
	if (php_mongo_get_cursor_id(result, &cursor_id TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 30 TSRMLS_CC, "the command cursor did not return a correctly structured response (no ID)");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), result TSRMLS_CC);
		zval_ptr_dtor(&result);
		return;
	}

	if (php_mongo_get_cursor_first_batch(result, &first_batch TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 31 TSRMLS_CC, "the command cursor did not return a correctly structured response (no first batch)");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), result TSRMLS_CC);
		zval_ptr_dtor(&result);
		return;
	}

	cmd_cursor->started_iterating = 1;
	cmd_cursor->cursor_id = cursor_id;
	cmd_cursor->first_batch = first_batch;
	Z_ADDREF_P(first_batch);
	cmd_cursor->first_batch_at = 0;
	cmd_cursor->first_batch_num = HASH_OF(cmd_cursor->first_batch)->nNumOfElements;

	/* We don't really *have* to return the value, but it makes testing easier,
	 * and perhaps diagnostics as well. */
	RETVAL_ZVAL(result, 0, 1);
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

/* {{{ MongoCommandCursor::reset()
   Does nothing yet. */
PHP_METHOD(MongoCommandCursor, reset)
{
	printf("reset\n");
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
	PHP_ME(MongoCommandCursor, reset, arginfo_no_parameters, ZEND_ACC_PUBLIC)

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
