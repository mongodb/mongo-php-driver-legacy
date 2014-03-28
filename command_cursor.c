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
#include "mcon/manager.h"

/* externs */
extern zend_class_entry *mongo_ce_MongoClient;
extern zend_class_entry *mongo_ce_CursorInterface;
extern zend_class_entry *mongo_ce_Exception, *mongo_ce_CursorException;
extern zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

zend_class_entry *mongo_ce_CommandCursor = NULL;

#define MONGO_DEFAULT_COMMAND_BATCH_SIZE 101

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

/* {{{ proto MongoCommandCursor::__construct(MongoClient connection, string ns, array query)
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

int php_mongo_enforce_batch_size_on_command(zval *command, int size TSRMLS_DC)
{
	zval **zcursor, **zbatchsize;

	if (Z_TYPE_P(command) != IS_ARRAY) {
		/* Technically we can not hit this, as the command should be an
		 * array, but this is just here as a precaution. */
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 32 TSRMLS_CC, "The cursor command structure is not an array");
		return 0;
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
		return 0;
	}

	if (zend_hash_find(HASH_PP(zcursor), "batchSize", 10, (void**) &zbatchsize) == FAILURE) {
		add_assoc_long(*zcursor, "batchSize", size);
	}

	return 1;
}

/* {{{ MongoCommandCursor iteration helpers */
static int php_mongocommandcursor_is_valid(mongo_command_cursor *cmd_cursor)
{
	return cmd_cursor->current != NULL;
}

static int php_mongocommandcursor_load_current_element(mongo_command_cursor *cmd_cursor TSRMLS_DC)
{
	/* Free the previous current item */
	if (cmd_cursor->current) {
		zval_ptr_dtor(&cmd_cursor->current);
		cmd_cursor->current = NULL;
	}
	
	/* Do processing of the first batch */
	if (cmd_cursor->first_batch) {
		zval **current;

		if (zend_hash_index_find(HASH_OF(cmd_cursor->first_batch), cmd_cursor->first_batch_at, (void**) &current) == SUCCESS) {
			cmd_cursor->current = *current;
			Z_ADDREF_PP(current);
			return SUCCESS;
		}
	}

	/* Do processing of subsequent batches, like a normal cursor */
	if (cmd_cursor->at >= cmd_cursor->num) {
		return FAILURE;
	}
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
		return FAILURE;
	}

	return SUCCESS;
}

static int php_mongocommandcursor_advance(mongo_command_cursor *cmd_cursor TSRMLS_DC)
{
	if (cmd_cursor->first_batch) {
		cmd_cursor->first_batch_at++;

		if (cmd_cursor->first_batch_at >= cmd_cursor->first_batch_num) {
			/* We're out of bounds for the first batch */
			zval_ptr_dtor(&cmd_cursor->first_batch);
			cmd_cursor->first_batch = NULL;

			/* Now see whether we have a cursor ID, and if we do, call getmore */
			if (cmd_cursor->cursor_id != 0) {
				if (!php_mongo_get_more(cmd_cursor TSRMLS_CC)) {
					return FAILURE;
				}
			}
		}
	} else {
		cmd_cursor->at++;

		if (cmd_cursor->at == cmd_cursor->num && cmd_cursor->cursor_id != 0) {
			if (!php_mongo_get_more(cmd_cursor TSRMLS_CC)) {
				cmd_cursor->cursor_id = 0;
				return FAILURE;
			}
		}
	}
	return php_mongocommandcursor_load_current_element(cmd_cursor TSRMLS_CC);
}
/* }}} */

/* {{{ proto array MongoCommandCursor::rewind()
   Resets the command cursor, executes the associated query and prepares the iterator. Returns the raw command document */
PHP_METHOD(MongoCommandCursor, rewind)
{
	char *dbname, *ns;
	zval *result;
	int64_t cursor_id;
	zval *exception;
	zval *first_batch;
	zval *cursor_env;
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	/* Shortcut in case we have a MongoCommandCursor created from a document. In this
	 * case we *can not* iterate twice */
	if (cmd_cursor->pre_created) {
		if (cmd_cursor->started_iterating == 1) {
			zend_throw_exception(mongo_ce_CursorException, "cannot iterate twice with command cursors created through createFromDocument", 33 TSRMLS_CC);
			return;
		}

		/* If the first batch is empty (as it is with parallelCollectionScan), then
		 * we already read the first batch here on rewind */
		if (cmd_cursor->first_batch_num == 0) {
			zval_ptr_dtor(&cmd_cursor->first_batch);
			cmd_cursor->first_batch = NULL;
			php_mongo_get_more((mongo_cursor*) cmd_cursor TSRMLS_CC);
		}
		php_mongocommandcursor_load_current_element(cmd_cursor TSRMLS_CC);

		cmd_cursor->started_iterating = 1;

		RETURN_TRUE;
	}

	php_mongo_cursor_reset((mongo_cursor*)cmd_cursor TSRMLS_CC);

	/* reads the batchsize through the command, or uses 101 when nothing is set */
	cmd_cursor->batch_size = get_batch_size_from_command(cmd_cursor->query TSRMLS_CC);
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

	/* We need to parse the initial result, and find the "cursor" element in the result */
	if (php_mongo_get_cursor_info_envelope(result, &cursor_env TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 30 TSRMLS_CC, "the command cursor did not return a correctly structured response");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), result TSRMLS_CC);
		zval_ptr_dtor(&result);
		return;
	}
	/* And from the envelope, we pick out the ID, namespace and first batch */
	if (php_mongo_get_cursor_info(cursor_env, &cursor_id, &ns, &first_batch TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 30 TSRMLS_CC, "the command cursor did not return a correctly structured response");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), result TSRMLS_CC);
		zval_ptr_dtor(&result);
		return;
	}

	cmd_cursor->started_iterating = 1;
	cmd_cursor->cursor_id = cursor_id;
	cmd_cursor->first_batch = first_batch;
	Z_ADDREF_P(first_batch);
	if (cmd_cursor->ns) {
		efree(cmd_cursor->ns);
	}
	cmd_cursor->ns = estrdup(ns);
	cmd_cursor->first_batch_at = 0;
	cmd_cursor->first_batch_num = HASH_OF(cmd_cursor->first_batch)->nNumOfElements;

	php_mongocommandcursor_load_current_element(cmd_cursor TSRMLS_CC);

	/* We don't really *have* to return the value, but it makes testing easier,
	 * and perhaps diagnostics as well. */
	RETVAL_ZVAL(result, 0, 1);
}
/* }}} */

/* {{{ proto bool MongoCommandCursor::valid()
   Returns whether the current iterator position is valid and fetches the key/value associated with the position. */
PHP_METHOD(MongoCommandCursor, valid)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	if (!cmd_cursor->started_iterating) {
		RETURN_FALSE;
	}

	if (!php_mongocommandcursor_is_valid(cmd_cursor)) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void MongoCommandCursor::next()
   Advances the interal cursor position. */
PHP_METHOD(MongoCommandCursor, next)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCommandCursor);

	php_mongocommandcursor_advance(cmd_cursor TSRMLS_CC);
}
/* }}} */

/* {{{ proto mixed MongoCommandCursor::current()
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

/* {{{ proto mixed MongoCommandCursor::key()
   Returns the numeric index of the current cursor position. */
PHP_METHOD(MongoCommandCursor, key)
{
	mongo_command_cursor *cmd_cursor = (mongo_command_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zval **id;

	MONGO_CHECK_INITIALIZED(cmd_cursor->zmongoclient, MongoCursor);

	if (!cmd_cursor->current) {
		RETURN_NULL();
	}

	if (cmd_cursor->first_batch) {
		RETURN_LONG(cmd_cursor->first_batch_at - 1);
	} else {
		RETURN_LONG(cmd_cursor->first_batch_num + cmd_cursor->at - 1);
	}
}
/* }}} */

void php_mongo_command_cursor_init_from_document(zval *zlink, mongo_command_cursor *cmd_cursor, char *hash, zval *document TSRMLS_DC)
{
	mongo_connection *connection;
	zval             *first_batch;
	char             *ns;
	int64_t           cursor_id;
	mongoclient      *link;
	zval             *exception;

	/* Find link */
	link = (mongoclient*)zend_object_store_get_object(zlink TSRMLS_CC);
	if (!link) {
		zend_throw_exception(mongo_ce_Exception, "The MongoCollection object has not been correctly initialized by its constructor", 17 TSRMLS_CC);
		return;
	}

	/* Find connection */
	connection = mongo_manager_connection_find_by_hash(link->manager, hash);
	if (!connection) {
		php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 21 TSRMLS_CC, "Cannot find connection associated with: '%s'", hash);
		return;
	}

	/* Get cursor info from array with ID, namespace and first batch elements */
	if (php_mongo_get_cursor_info(document, &cursor_id, &ns, &first_batch TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 30 TSRMLS_CC, "the command cursor did not return a correctly structured response");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), document TSRMLS_CC);
		return;
	}

	/* ns */
	cmd_cursor->ns = estrdup(ns);

	/* db connection */
	cmd_cursor->zmongoclient = zlink;
	zval_add_ref(&zlink);

	/* query */
	cmd_cursor->query = NULL;

	/* reset iteration pointer and flags */
	php_mongo_cursor_reset((mongo_cursor*)cmd_cursor TSRMLS_CC);
	cmd_cursor->special = 0;

	/* default cursor properties */
	cmd_cursor->connection = connection;
	cmd_cursor->cursor_id = cursor_id;
	cmd_cursor->first_batch = first_batch;
	Z_ADDREF_P(first_batch);
	cmd_cursor->first_batch_at = 0;
	cmd_cursor->first_batch_num = HASH_OF(cmd_cursor->first_batch)->nNumOfElements;

	/* flags */
	php_mongo_cursor_force_command_cursor(cmd_cursor);
	cmd_cursor->pre_created = 1;
}

static zval *php_mongo_commandcursor_instantiate(zval *object TSRMLS_DC)
{
	Z_TYPE_P(object) = IS_OBJECT;
	object_init_ex(object, mongo_ce_CommandCursor);
	Z_SET_REFCOUNT_P(object, 1);
	Z_UNSET_ISREF_P(object);

	return object;
}

/* {{{ proto MongoCommandCursor::createFromDocument(MongoClient connection, string hash, array document)
   Constructs a MongoCommandCursor from a cursor document */
PHP_METHOD(MongoCommandCursor, createFromDocument)
{
	zval *zlink = NULL, *document = NULL;
	char *hash;
	int   hash_len;
	mongo_command_cursor *cmd_cursor;
	mongoclient *link;
	zval *cursor_env;
	zval *exception;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Osa", &zlink, mongo_ce_MongoClient, &hash, &hash_len, &document) == FAILURE) {
		return;
	}

	link = (mongoclient*)zend_object_store_get_object(zlink TSRMLS_CC);
	if (!link) {
		zend_throw_exception(mongo_ce_Exception, "The MongoCollection object has not been correctly initialized by its constructor", 17 TSRMLS_CC);
		return;
	}

	php_mongo_commandcursor_instantiate(return_value TSRMLS_CC);
	cmd_cursor = (mongo_command_cursor*) zend_object_store_get_object(return_value TSRMLS_CC);

	/* We need to parse the initial result, and find the "cursor" element in the result */
	if (php_mongo_get_cursor_info_envelope(document, &cursor_env TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 30 TSRMLS_CC, "the command cursor did not return a correctly structured response");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), document TSRMLS_CC);
		return;
	}

	php_mongo_command_cursor_init_from_document(zlink, cmd_cursor, hash, cursor_env TSRMLS_CC);

}
/* }}} */

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

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_createfromdocument, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_OBJ_INFO(0, connection, MongoClient, 0)
	ZEND_ARG_INFO(0, connection_hash)
	ZEND_ARG_INFO(0, cursor_document)
ZEND_END_ARG_INFO()


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

	/* factory methods */
	PHP_ME(MongoCommandCursor, createFromDocument, arginfo_createfromdocument, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)

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
