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
#include <ext/standard/md5.h>
#include <ext/standard/php_smart_str.h>

#include "php_mongo.h"

#include "db.h"
#include "collection.h"
#include "cursor.h"
#include "command_cursor.h"
#include "cursor_shared.h"
#include "gridfs/gridfs.h"
#include "types/code.h"
#include "types/db_ref.h"
#include "mcon/manager.h"
#include "api/wire_version.h"

#ifndef zend_parse_parameters_none
#define zend_parse_parameters_none()    \
        zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")
#endif

extern zend_class_entry *mongo_ce_MongoClient, *mongo_ce_Collection;
extern zend_class_entry *mongo_ce_Cursor, *mongo_ce_GridFS, *mongo_ce_Id;
extern zend_class_entry *mongo_ce_Code, *mongo_ce_Exception;
extern zend_class_entry *mongo_ce_CursorException, *mongo_ce_Int64;
extern zend_class_entry *mongo_ce_ConnectionException, *mongo_ce_ResultException;

extern zend_object_handlers mongo_default_handlers;

zend_class_entry *mongo_ce_DB = NULL;

static void clear_exception(zval* return_value TSRMLS_DC);

static int php_mongo_command_supports_rp(zval *cmd)
{
	HashPosition pos;
	char *str;
	uint str_len;
	long type;
	ulong idx;

	if (!cmd || Z_TYPE_P(cmd) != IS_ARRAY) {
		return 0;
	}

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(cmd), &pos);
	type = zend_hash_get_current_key_ex(Z_ARRVAL_P(cmd), &str, &str_len, &idx, 0, &pos);
	if (type != HASH_KEY_IS_STRING) {
		return 0;
	}

	/* Commands in MongoDB are case-sensitive */
	if (str_len == 6) {
		if (strcmp(str, "count") == 0 || strcmp(str, "group") == 0) {
			return 1;
		}
		return 0;
	}
	if (str_len == 8) {
		if (strcmp(str, "dbStats") == 0 || strcmp(str, "geoNear") == 0 || strcmp(str, "geoWalk") == 0) {
			return 1;
		}
		return 0;
	}
	if (str_len == 9) {
		if (strcmp(str, "distinct") == 0) {
			return 1;
		}
		return 0;
	}
	if (str_len == 10) {
		if (strcmp(str, "aggregate") == 0 || strcmp(str, "collStats") == 0 || strcmp(str, "geoSearch") == 0) {
			return 1;
		}

		if (strcmp(str, "mapreduce") == 0 || strcmp(str, "mapReduce") == 0) {
			zval **value = NULL;
			if (zend_hash_find(Z_ARRVAL_P(cmd), "out", 4, (void **)&value) == SUCCESS) {
				if (Z_TYPE_PP(value) == IS_ARRAY) {
					if (zend_hash_exists(Z_ARRVAL_PP(value), "inline", 7)) {
						return 1;
					}
				}
			}
		}
		return 0;
	}
	if (str_len == 23) {
		if (strcmp(str, "parallelCollectionScan") == 0) {
			return 1;
		}
		return 0;
	}

	return 0;
}

int php_mongo_db_is_valid_dbname(char *dbname, int dbname_len TSRMLS_DC)
{
	if (dbname_len == 0) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "Database name cannot be empty");
		return 0;
	}

	if (dbname_len >= 64) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "Database name cannot exceed 63 characters: %s", dbname);
		return 0;
	}

	if (memchr(dbname, '\0', dbname_len) != NULL) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "Database name cannot contain null bytes: %s\\0...", dbname);
		return 0;
	}

	/* We allow the special case "$external" as database name (PHP-1431) */
	if (strcmp("$external", dbname) == 0) {
		return 1;
	}

	if (
		memchr(dbname, ' ', dbname_len) != 0 || memchr(dbname, '.', dbname_len) != 0 || memchr(dbname, '\\', dbname_len) != 0 ||
		memchr(dbname, '/', dbname_len) != 0 || memchr(dbname, '$', dbname_len) != 0
	) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "Database name contains invalid characters: %s", dbname);
		return 0;
	}

	return 1;
}


void php_mongo_db_construct(zval *z_client, zval *zlink, char *name, int name_len TSRMLS_DC)
{
	mongo_db *db;
	mongoclient *link;

	if (!php_mongo_db_is_valid_dbname(name, name_len TSRMLS_CC)) {
		return;
	}

	db = (mongo_db*)zend_object_store_get_object(z_client TSRMLS_CC);

	db->link = zlink;
	zval_add_ref(&db->link);

	link = (mongoclient*)zend_object_store_get_object(zlink TSRMLS_CC);
	if (!(link->servers)) {
		zend_throw_exception(mongo_ce_Exception, "The MongoDB object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
		return;
	}

	if (link->servers->options.default_w != -1) {
		zend_update_property_long(mongo_ce_DB, z_client, "w", strlen("w"), link->servers->options.default_w TSRMLS_CC);
	} else if (link->servers->options.default_wstring != NULL) {
		zend_update_property_string(mongo_ce_DB, z_client, "w", strlen("w"), link->servers->options.default_wstring TSRMLS_CC);
	}
	if (link->servers->options.default_wtimeout != -1) {
		zend_update_property_long(mongo_ce_DB, z_client, "wtimeout", strlen("wtimeout"), link->servers->options.default_wtimeout TSRMLS_CC);
	}
	mongo_read_preference_copy(&link->servers->read_pref, &db->read_pref);

	MAKE_STD_ZVAL(db->name);
	ZVAL_STRING(db->name, name, 1);
}

/* {{{ proto void MongoDB::__construct(MongoClient client, string name)
   Constructs a new MongoDB instance */
PHP_METHOD(MongoDB, __construct)
{
	zval *zlink;
	char *name;
	int name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &zlink, mongo_ce_MongoClient, &name, &name_len) == FAILURE) {
		zval *object = getThis();
		ZVAL_NULL(object);
		return;
	}

	php_mongo_db_construct(getThis(), zlink, name, name_len TSRMLS_CC);
}
/* }}} */

PHP_METHOD(MongoDB, __toString)
{
	mongo_db *db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED_STRING(db->name, MongoDB);
	RETURN_ZVAL(db->name, 1, 0);
}

/* Selects a collection and returns it as zval. If the return value is no, an
 * Exception is set. This only happens if the passed in DB was invalid. */
zval *php_mongo_db_selectcollection(zval *z_client, char *collection, int collection_len TSRMLS_DC)
{
	zval *z_collection;
	zval *return_value;
	mongo_db *db;

	db = (mongo_db*)zend_object_store_get_object(z_client TSRMLS_CC);
	if (!(db->name)) {
		zend_throw_exception(mongo_ce_Exception, "The MongoDB object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
		return NULL;
	}

	MAKE_STD_ZVAL(z_collection);
	ZVAL_STRINGL(z_collection, collection, collection_len, 1);

	MAKE_STD_ZVAL(return_value);
	object_init_ex(return_value, mongo_ce_Collection);

	php_mongo_collection_construct(return_value, z_client, collection, collection_len TSRMLS_CC);
	if (EG(exception)) {
		zval_ptr_dtor(&return_value);
		return_value = NULL;
	}

	zval_ptr_dtor(&z_collection);

	return return_value;
}

/* {{{ proto MongoCollection MongoDB::selectCollection(string name)
   Returns the "name" collection from the database */
PHP_METHOD(MongoDB, selectCollection)
{
	char *name;
	int   name_len;
	zval *collection;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		return;
	}

	collection = php_mongo_db_selectcollection(getThis(), name, name_len TSRMLS_CC);
	if (collection) {
		/* Only copy the zval into return_value if it worked. If collection is
		 * NULL here, an exception is set */
		RETURN_ZVAL(collection, 0, 1);
	}
}
/* }}} */

PHP_METHOD(MongoDB, getGridFS)
{
	zval temp;
	zval *arg1 = 0, *arg2 = 0;

	/* arg2 is deprecated */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &arg1, &arg2) == FAILURE) {
		return;
	}
	if (arg2) {
		php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'chunks' argument is deprecated and ignored");
	}

	object_init_ex(return_value, mongo_ce_GridFS);

	if (!arg1) {
		MONGO_METHOD1(MongoGridFS, __construct, &temp, return_value, getThis());
	} else {
		MONGO_METHOD2(MongoGridFS, __construct, &temp, return_value, getThis(), arg1);
	}
}

PHP_METHOD(MongoDB, getSlaveOkay)
{
	mongo_db *db;
	PHP_MONGO_GET_DB(getThis());
	RETURN_BOOL(db->read_pref.type != MONGO_RP_PRIMARY);
}

PHP_METHOD(MongoDB, setSlaveOkay)
{
	zend_bool slave_okay = 1;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &slave_okay) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	RETVAL_BOOL(db->read_pref.type != MONGO_RP_PRIMARY);
	db->read_pref.type = slave_okay ? MONGO_RP_SECONDARY_PREFERRED : MONGO_RP_PRIMARY;
}


PHP_METHOD(MongoDB, getReadPreference)
{
	mongo_db *db;
	PHP_MONGO_GET_DB(getThis());

	array_init(return_value);
	add_assoc_string(return_value, "type", mongo_read_preference_type_to_name(db->read_pref.type), 1);
	php_mongo_add_tagsets(return_value, &db->read_pref);
}

/* {{{ MongoDB::setReadPreference(string read_preference [, array tags ])
 * Sets a read preference to be used for all read queries.*/
PHP_METHOD(MongoDB, setReadPreference)
{
	char *read_preference;
	int   read_preference_len;
	mongo_db *db;
	HashTable  *tags = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|h", &read_preference, &read_preference_len, &tags) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	if (php_mongo_set_readpreference(&db->read_pref, read_preference, tags TSRMLS_CC)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ array MongoDB::getWriteConcern()
 * Get the MongoDB write concern, which will be inherited by constructed
 * MongoCollection objects. */
PHP_METHOD(MongoDB, getWriteConcern)
{
	zval *write_concern, *wtimeout;

	if (zend_parse_parameters_none()) {
		return;
	}

	write_concern = zend_read_property(mongo_ce_DB, getThis(), "w", strlen("w"), 0 TSRMLS_CC);
	wtimeout      = zend_read_property(mongo_ce_DB, getThis(), "wtimeout", strlen("wtimeout"), 0 TSRMLS_CC);
	Z_ADDREF_P(write_concern);
	Z_ADDREF_P(wtimeout);

	array_init(return_value);
	add_assoc_zval(return_value, "w", write_concern);
	add_assoc_zval(return_value, "wtimeout", wtimeout);
}
/* }}} */

/* {{{ bool MongoDB::setWriteConcern(mixed w [, int wtimeout])
 * Set the MongoDB write concern, which will be inherited by constructed
 * MongoCollection objects. */
PHP_METHOD(MongoDB, setWriteConcern)
{
	zval *write_concern;
	long  wtimeout;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", &write_concern, &wtimeout) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(write_concern) == IS_LONG) {
		zend_update_property_long(mongo_ce_DB, getThis(), "w", strlen("w"), Z_LVAL_P(write_concern) TSRMLS_CC);
	} else if (Z_TYPE_P(write_concern) == IS_STRING) {
		zend_update_property_stringl(mongo_ce_DB, getThis(), "w", strlen("w"), Z_STRVAL_P(write_concern), Z_STRLEN_P(write_concern) TSRMLS_CC);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects parameter 1 to be an string or integer, %s given", zend_get_type_by_const(Z_TYPE_P(write_concern)));
		RETURN_FALSE;
	}

	if (ZEND_NUM_ARGS() > 1) {
		zend_update_property_long(mongo_ce_DB, getThis(), "wtimeout", strlen("wtimeout"), wtimeout TSRMLS_CC);
	}

	RETURN_TRUE;
}
/* }}} */

static void php_mongo_db_profiling_level(INTERNAL_FUNCTION_PARAMETERS, int get)
{
	long level;
	zval *cmd, *cmd_return;
	zval **ok;
	mongo_db *db;

	if (get) {
		if (zend_parse_parameters_none() == FAILURE) {
			return;
		}
		level = -1;
	} else {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &level) == FAILURE) {
			return;
		}
	}

	PHP_MONGO_GET_DB(getThis());

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_long(cmd, "profile", level);

	cmd_return = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);

	if (!cmd_return) {
		return;
	}

	if (
		zend_hash_find(HASH_P(cmd_return), "ok", 3, (void**)&ok) == SUCCESS &&
		((Z_TYPE_PP(ok) == IS_BOOL && Z_BVAL_PP(ok)) || Z_DVAL_PP(ok) == 1)
	) {
		zend_hash_find(HASH_P(cmd_return), "was", 4, (void**)&ok);
		RETVAL_ZVAL(*ok, 1, 0);
	} else {
		RETVAL_NULL();
	}
	zval_ptr_dtor(&cmd_return);
}

/* {{{ proto array MongoDB::getProfilingLevel()
   Gets this database's profiling level. */
PHP_METHOD(MongoDB, getProfilingLevel)
{
	php_mongo_db_profiling_level(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto array MongoDB::setProfilingLevel(int level)
   Sets this database's profiling level. */
PHP_METHOD(MongoDB, setProfilingLevel)
{
	php_mongo_db_profiling_level(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

PHP_METHOD(MongoDB, drop)
{
	zval *cmd, *retval;
	mongo_db *db;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_long(cmd, "dropDatabase", 1);

	retval = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);

	if (retval) {
		RETURN_ZVAL(retval, 0, 1);
	}
}

PHP_METHOD(MongoDB, repair)
{
	zend_bool cloned=0, original=0;
	zval *cmd, *retval;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|bb", &cloned, &original) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_long(cmd, "repairDatabase", 1);
	add_assoc_bool(cmd, "preserveClonedFilesOnFailure", cloned);
	add_assoc_bool(cmd, "backupOriginalFiles", original);

	retval = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);

	if (retval) {
		RETVAL_ZVAL(retval, 0, 1);
	}
}


PHP_METHOD(MongoDB, createCollection)
{
	zval *cmd = NULL, *temp, *options = NULL;
	char *collection;
	int   collection_len;
	zend_bool capped = 0;
	long size = 0, max = 0;
	mongo_db *db;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "s|bll", &collection, &collection_len, &capped, &size, &max) == SUCCESS) {
		MAKE_STD_ZVAL(cmd);
		array_init(cmd);

		add_assoc_stringl(cmd, "create", collection, collection_len, 1);

		if (size) {
			add_assoc_long(cmd, "size", size);
		}

		if (capped) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "This method now accepts arguments as an options array instead of the three optional arguments for capped, size and max elements");
			add_assoc_bool(cmd, "capped", 1);
			if (max) {
				add_assoc_long(cmd, "max", max);
			}
		}

	} else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &collection, &collection_len, &options) == SUCCESS) {
		zval *tmp_copy;

		/* We create a new array here, instead of just tagging "create" =>
		 * <name> at the end of the array. This is because MongoDB wants the
		 * name of the command as first element in the array. */
		MAKE_STD_ZVAL(cmd);
		array_init(cmd);
		add_assoc_stringl(cmd, "create", collection, collection_len, 1);
		if (options) {
			zend_hash_merge(Z_ARRVAL_P(cmd), Z_ARRVAL_P(options), (copy_ctor_func_t) zval_add_ref, (void *) &tmp_copy, sizeof(zval *), 0);
		}
	} else {

		return;
	}

	PHP_MONGO_GET_DB(getThis());

	temp = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, options, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);
	if (temp) {
		zval_ptr_dtor(&temp);
	}

	if (!EG(exception)) {
		zval *zcollection;

		/* get the collection we just created */
		zcollection = php_mongo_db_selectcollection(getThis(), collection, collection_len TSRMLS_CC);
		if (zcollection) {
			/* Only copy the zval into return_value if it worked. If
			 * zcollection is NULL here, an exception is set */
			RETURN_ZVAL(zcollection, 0, 1);
		}
	}
}

/* {{{ proto MongoCollection MongoDB::dropCollection(string|MongoCollection collection)
   Drops a collection and returns the database's response */
PHP_METHOD(MongoDB, dropCollection)
{
	zval *collection;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &collection) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(collection) == IS_STRING) {
		collection = php_mongo_db_selectcollection(getThis(), Z_STRVAL_P(collection), Z_STRLEN_P(collection) TSRMLS_CC);
		if (!collection) {
			/* An exception is set in this case */
			return;
		}
	} else if (Z_TYPE_P(collection) == IS_OBJECT && Z_OBJCE_P(collection) == mongo_ce_Collection) {
		zval_add_ref(&collection);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects parameter 1 to be an string or MongoCollection");
		return;
	}

	php_mongocollection_drop(collection, return_value TSRMLS_CC);

	zval_ptr_dtor(&collection);
}
/* }}} */

static void mongo_db_list_collections_command(zval *this_ptr, zval *options, int return_type,  zval *return_value TSRMLS_DC)
{
	zend_bool include_system_collections = 0;
	zval *z_cmd, *list, **collections;
	mongo_db *db;
	mongo_connection *connection;
	zval *cursor_env, *retval;
	zval *tmp_iterator, *exception;
	mongo_cursor *cmd_cursor;

	MAKE_STD_ZVAL(z_cmd);
	array_init(z_cmd);
	add_assoc_long(z_cmd, "listCollections", 1);

	if (options) {
		zval *temp, **include_system_collections_pp;

		/* "includeSystemCollections" should not be included in the command document */
		if (zend_hash_find(HASH_P(options), ZEND_STRS("includeSystemCollections"), (void**)&include_system_collections_pp) == SUCCESS) {
			convert_to_boolean(*include_system_collections_pp);
			include_system_collections = Z_BVAL_PP(include_system_collections_pp);
			zend_hash_del(HASH_P(options), "includeSystemCollections", strlen("includeSystemCollections") + 1);
		}

		zend_hash_merge(HASH_P(z_cmd), HASH_P(options), (copy_ctor_func_t) zval_add_ref, &temp, sizeof(zval*), 1);
	}

	/* Ensure that the command's cursor option, if set, is valid. If this fails,
	 * EG(exception) is set. */
	if (!php_mongo_validate_cursor_on_command(z_cmd TSRMLS_CC)) {
		zval_ptr_dtor(&z_cmd);
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	retval = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), z_cmd, NULL, 0, &connection TSRMLS_CC);
	
	zval_ptr_dtor(&z_cmd);
	
	if (!retval) {
		return;
	}

	if (php_mongo_trigger_error_on_command_failure(connection, retval TSRMLS_CC) == FAILURE) {
		RETURN_ZVAL(retval, 0, 1);
	}

	/* list to return */
	MAKE_STD_ZVAL(list);
	array_init(list);

	/* Handle inline command response from server >= 2.7.5 and < 2.8.0-RC3. */
	if (zend_hash_find(Z_ARRVAL_P(retval), "collections", strlen("collections") + 1, (void **)&collections) == SUCCESS) {
		HashPosition pointer;
		zval **collection_doc;

		for (
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(collections), &pointer);
			zend_hash_get_current_data_ex(Z_ARRVAL_PP(collections), (void**)&collection_doc, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(Z_ARRVAL_PP(collections), &pointer)
		) {
			zval *c;
			zval **collection_name;
			char *system;

			if (zend_hash_find(Z_ARRVAL_PP(collection_doc), "name", 5, (void**)&collection_name) == FAILURE) {
				continue;
			}

			/* check that this isn't a system ns */
			system = strstr(Z_STRVAL_PP(collection_name), "system.");
			if (
				(!include_system_collections && (system == Z_STRVAL_PP(collection_name)))
			) {
				continue;
			}

			switch (return_type) {
				case MONGO_COLLECTION_RETURN_TYPE_NAME:
					add_next_index_string(list, Z_STRVAL_PP(collection_name), 1);
					break;

				case MONGO_COLLECTION_RETURN_TYPE_OBJECT:
					c = php_mongo_db_selectcollection(this_ptr, Z_STRVAL_PP(collection_name), Z_STRLEN_PP(collection_name) TSRMLS_CC);
					add_next_index_zval(list, c);
					break;

				case MONGO_COLLECTION_RETURN_TYPE_INFO_ARRAY:
					Z_ADDREF_P(*collection_doc);
					add_assoc_zval(list, Z_STRVAL_PP(collection_name), *collection_doc);
					break;
			}
		}

		zval_ptr_dtor(&retval);

		RETURN_ZVAL(list, 0, 1);
	}

	MAKE_STD_ZVAL(tmp_iterator);
	php_mongo_commandcursor_instantiate(tmp_iterator TSRMLS_CC);
	cmd_cursor = (mongo_command_cursor*) zend_object_store_get_object(tmp_iterator TSRMLS_CC);

	/* We need to parse the initial result, and find the "cursor" element in the result */
	if (php_mongo_get_cursor_info_envelope(retval, &cursor_env TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 30 TSRMLS_CC, "the command cursor did not return a correctly structured response");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), retval TSRMLS_CC);
		zval_ptr_dtor(&tmp_iterator);
		return;
	}

	php_mongo_command_cursor_init_from_document(db->link, cmd_cursor, connection->hash, cursor_env TSRMLS_CC);

	/* TODO: Refactor to use MongoCommandCursor::rewind() once it is extracted
	 * into its own C function (see PHP-1362) */
	php_mongocommandcursor_fetch_batch_if_first_is_empty(cmd_cursor TSRMLS_CC);
	cmd_cursor->started_iterating = 1;

	for (
		php_mongocommandcursor_load_current_element(cmd_cursor TSRMLS_CC);
		php_mongocommandcursor_is_valid(cmd_cursor);
		php_mongocommandcursor_advance(cmd_cursor TSRMLS_CC)
	) {
		zval **collection_name;
		zval **collection_doc = &cmd_cursor->current;
		char *system;

		if (zend_hash_find(Z_ARRVAL_PP(collection_doc), "name", 5, (void**)&collection_name) == FAILURE) {
			continue;
		}

		/* check that this isn't a system ns */
		system = strstr(Z_STRVAL_PP(collection_name), "system.");
		if (
			(!include_system_collections && (system == Z_STRVAL_PP(collection_name)))
		) {
			continue;
		}

		switch (return_type) {
			case MONGO_COLLECTION_RETURN_TYPE_NAME:
				add_next_index_string(list, Z_STRVAL_PP(collection_name), 1);
				break;

			case MONGO_COLLECTION_RETURN_TYPE_OBJECT: {
				zval *c = php_mongo_db_selectcollection(this_ptr, Z_STRVAL_PP(collection_name), Z_STRLEN_PP(collection_name) TSRMLS_CC);
				add_next_index_zval(list, c);
				break;
			}

			case MONGO_COLLECTION_RETURN_TYPE_INFO_ARRAY:
				Z_ADDREF_P(*collection_doc);
				add_next_index_zval(list, *collection_doc);
				break;
		}
	}

	zval_ptr_dtor(&retval);
	zval_ptr_dtor(&tmp_iterator);

	RETURN_ZVAL(list, 0, 1);
}

static void mongo_db_list_collections_legacy(zval *this_ptr, zval *options, int return_type, zval *return_value TSRMLS_DC)
{
	zend_bool include_system_collections = 0;
	zval *z_system_collection, *z_cursor, *list, *filter = NULL;
	mongo_cursor *cursor;
	mongo_collection *collection;

	if (options) {
		zval **include_system_collections_pp, **filter_pp;

		if (zend_hash_find(HASH_P(options), ZEND_STRS("includeSystemCollections"), (void**)&include_system_collections_pp) == SUCCESS) {
			convert_to_boolean(*include_system_collections_pp);
			include_system_collections = Z_BVAL_PP(include_system_collections_pp);
		}

		if (zend_hash_find(HASH_P(options), ZEND_STRS("filter"), (void**)&filter_pp) == SUCCESS) {
			if (Z_TYPE_PP(filter_pp) != IS_ARRAY && Z_TYPE_PP(filter_pp) != IS_OBJECT) {
				zend_throw_exception_ex(mongo_ce_Exception, 26 TSRMLS_CC, "Expected filter to be array or object, %s given", zend_get_type_by_const(Z_TYPE_PP(filter_pp)));
				RETURN_NULL();
			}

			filter = *filter_pp;
		}
	}

	if (filter) {
		zval **name_pp;

		if (zend_hash_find(HASH_P(filter), ZEND_STRS("name"), (void**)&name_pp) == SUCCESS) {
			mongo_db *db;
			int prefixed_name_len;
			char *prefixed_name;

			if (Z_TYPE_PP(name_pp) != IS_STRING) {
				zend_throw_exception_ex(mongo_ce_Exception, 27 TSRMLS_CC, "Filter \"name\" must be a string for MongoDB <2.8, %s given", zend_get_type_by_const(Z_TYPE_PP(name_pp)));
				RETURN_NULL();
			}

			db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);

			if (!(db->name)) {
				zend_throw_exception(mongo_ce_Exception, "The MongoDB object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
				RETURN_NULL();
			}

			prefixed_name_len = spprintf(&prefixed_name, 0, "%s.%s", Z_STRVAL_P(db->name), Z_STRVAL_PP(name_pp));
			add_assoc_stringl(filter, "name", prefixed_name, prefixed_name_len, 1);
			efree(prefixed_name);
		}
	}

	/* select db.system.namespaces collection */
	z_system_collection = php_mongo_db_selectcollection(this_ptr, "system.namespaces", strlen("system.namespaces") TSRMLS_CC);
	if (!z_system_collection) {
		/* An exception is set in this case */
		return;
	}

	/* list to return */
	MAKE_STD_ZVAL(list);
	array_init(list);

	/* do find */
	MAKE_STD_ZVAL(z_cursor);
	object_init_ex(z_cursor, mongo_ce_Cursor);
	cursor = (mongo_cursor*)zend_object_store_get_object(z_cursor TSRMLS_CC);
	collection = (mongo_collection*)zend_object_store_get_object(z_system_collection TSRMLS_CC);

	php_mongo_collection_find(cursor, collection, filter, NULL TSRMLS_CC);

	php_mongo_runquery(cursor TSRMLS_CC);
	if (EG(exception)) {
		zval_ptr_dtor(&z_cursor);
		zval_ptr_dtor(&z_system_collection);
		zval_ptr_dtor(&list);
		RETURN_NULL();
	}

	/* populate list */
	php_mongocursor_load_current_element(cursor TSRMLS_CC);

	if (php_mongo_handle_error(cursor TSRMLS_CC)) {
		zval_ptr_dtor(&z_cursor);
		zval_ptr_dtor(&z_system_collection);
		zval_ptr_dtor(&list);
		RETURN_NULL();
	}

	while (php_mongocursor_is_valid(cursor)) {
		zval *c;
		zval **collection_name;
		char *name, *first_dot, *system;

		/* check that the ns is valid and not an index (contains $) */
		if (
			zend_hash_find(HASH_P(cursor->current), "name", 5, (void**)&collection_name) == FAILURE ||
			(
				Z_TYPE_PP(collection_name) == IS_STRING &&
				strchr(Z_STRVAL_PP(collection_name), '$')
			)
		) {
			php_mongocursor_advance(cursor TSRMLS_CC);
			continue;
		}

		/* check that this isn't a system ns */
		first_dot = strchr(Z_STRVAL_PP(collection_name), '.');
		system = strstr(Z_STRVAL_PP(collection_name), ".system.");
		if (
			(!include_system_collections && (system && first_dot == system)) ||
			(name = strchr(Z_STRVAL_PP(collection_name), '.')) == 0)
		{
			php_mongocursor_advance(cursor TSRMLS_CC);
			continue;
		}

		/* take a substring after the first "." */
		name++;

		/* "foo." was allowed in earlier versions */
		if (*name == '\0') {
			php_mongocursor_advance(cursor TSRMLS_CC);
			continue;
		}

		switch (return_type) {
			case MONGO_COLLECTION_RETURN_TYPE_NAME:
				add_next_index_string(list, name, 1);
				break;

			case MONGO_COLLECTION_RETURN_TYPE_OBJECT:
				c = php_mongo_db_selectcollection(this_ptr, name, strlen(name) TSRMLS_CC);
				/* No need to test for c here, as this was already covered in
				 * system_collection above */
				add_next_index_zval(list, c);
				break;

			case MONGO_COLLECTION_RETURN_TYPE_INFO_ARRAY:
				Z_ADDREF_P(cursor->current);
				add_next_index_zval(list, cursor->current);
				/* Replace the collection's namespace with the trimmed name */
				add_assoc_string(cursor->current, "name", name, 1);
				break;
		}

		php_mongocursor_advance(cursor TSRMLS_CC);
	}

	zval_ptr_dtor(&z_cursor);
	zval_ptr_dtor(&z_system_collection);

	RETURN_ZVAL(list, 0, 1);
}

/* Return types:
 * 0: Collection name
 * 1: MongoCollection object
 * 2: Array containing collection name and options
 */
static void php_mongo_enumerate_collections(INTERNAL_FUNCTION_PARAMETERS, int return_type)
{
	zval *options = NULL;
	mongo_connection *connection;
	mongo_db *db;
	mongoclient *link;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &options) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());
	PHP_MONGO_GET_LINK(db->link);

	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	/* Convert non-hash parameter to "includeSystemCollections" option */
	if (options && Z_TYPE_P(options) != IS_ARRAY && Z_TYPE_P(options) != IS_OBJECT) {
		zend_bool include_system_collections;

		convert_to_boolean(options);
		include_system_collections = Z_BVAL_P(options);

		array_init(options);
		add_assoc_bool(options, "includeSystemCollections", include_system_collections);
	}

	if (php_mongo_api_connection_min_server_version(connection, 2, 7, 5)) {
		mongo_db_list_collections_command(getThis(), options, return_type, return_value TSRMLS_CC);
	} else {
		mongo_db_list_collections_legacy(getThis(), options, return_type, return_value TSRMLS_CC);
	}
}

PHP_METHOD(MongoDB, listCollections)
{
	php_mongo_enumerate_collections(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

PHP_METHOD(MongoDB, getCollectionInfo)
{
	php_mongo_enumerate_collections(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2);
}

PHP_METHOD(MongoDB, getCollectionNames)
{
	php_mongo_enumerate_collections(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

PHP_METHOD(MongoDB, createDBRef)
{
	char *collection;
	int collection_len;
	zval *obj, *retval = NULL;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &collection, &collection_len, &obj) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	if (
		(obj = php_mongo_dbref_resolve_id(obj TSRMLS_CC)) &&
		(retval = php_mongo_dbref_create(obj, collection, NULL TSRMLS_CC))
	) {
		RETURN_ZVAL(retval, 0, 1);
	}

	RETURN_NULL();
}

PHP_METHOD(MongoDB, getDBRef)
{
	zval *ref;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(1, ref);

	php_mongo_dbref_get(getThis(), ref, return_value TSRMLS_CC);
}

PHP_METHOD(MongoDB, execute)
{
	zval *code = NULL, *args = NULL, *options = NULL;
	zval *cmd, *retval;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|aa", &code, &args, &options) == FAILURE) {
		return;
	}

	/* turn the first argument into MongoCode */
	if (Z_TYPE_P(code) != IS_OBJECT ||
		Z_OBJCE_P(code) != mongo_ce_Code) {
		if (Z_TYPE_P(code) == IS_STRING) {
			zval *obj;

			MAKE_STD_ZVAL(obj);
			object_init_ex(obj, mongo_ce_Code);
			php_mongocode_populate(obj, Z_STRVAL_P(code), Z_STRLEN_P(code), NULL TSRMLS_CC);
			code = obj;
		} else { /* This is broken code */
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "The argument is neither an object of MongoCode or a string");
			return;
		}
	} else {
		zval_add_ref(&code);
	}

	if (!args) {
		MAKE_STD_ZVAL(args);
		array_init(args);
	} else {
		zval_add_ref(&args);
	}

	/* create { $eval : code, args : [] } */
	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_zval(cmd, "$eval", code);
	add_assoc_zval(cmd, "args", args);
	/* Check whether we have nolock as an option */
	if (options) {
		zval **nolock;
	
		if (zend_hash_find(HASH_P(options), "nolock", strlen("nolock") + 1, (void**) &nolock) == SUCCESS) {
			convert_to_boolean_ex(nolock);
			zval_add_ref(nolock);
			add_assoc_zval(cmd, "nolock", *nolock);
		}
	}

	PHP_MONGO_GET_DB(getThis());
	retval = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, options, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);
	if (retval) {
		RETURN_ZVAL(retval, 0, 1);
	}
}

static char *get_cmd_ns(char *db, int db_len)
{
	char *position;
	char *cmd_ns = (char*)emalloc(db_len + strlen("$cmd") + 2);

	position = cmd_ns;

	/* db */
	memcpy(position, db, db_len);
	position += db_len;

	/* . */
	*(position)++ = '.';

	/* $cmd */
	memcpy(position, "$cmd", strlen("$cmd"));
	position += strlen("$cmd");

	/* \0 */
	*(position) = '\0';

	return cmd_ns;
}

/* {{{ proto array MongoDB::command(array cmd [, array options = null [, string &hash]])
   Executes a database command and stores the used connection's hash in $hash. */
PHP_METHOD(MongoDB, command)
{
	zval *cmd, *retval, *options = NULL, *hash = NULL;
	mongo_connection *used_connection = NULL;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a!z", &cmd, &options, &hash) == FAILURE) {
		return;
	}

	MUST_BE_ARRAY_OR_OBJECT(1, cmd);

	PHP_MONGO_GET_DB(getThis());
	retval = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, options, 0, &used_connection TSRMLS_CC);

	if (used_connection && ZEND_NUM_ARGS() >= 3) {
		zval_dtor(hash);
		ZVAL_STRING(hash, used_connection->hash, 1);
	}

	if (retval) {
		RETVAL_ZVAL(retval, 0, 1);
	}
}

/* Actually execute the command after doing a few extra checks.
 *
 * This function can return NULL but *only* if an exception is set. So please
 * check for NULL and/or EG(exception) in the calling function. */
zval *php_mongo_runcommand(zval *zmongoclient, mongo_read_preference *read_preferences, char *dbname, int dbname_len, zval *cmd, zval *options, int is_cmd_cursor, mongo_connection **used_connection TSRMLS_DC)
{
	zval *temp, *cursor, *ns, *retval = NULL;
	mongo_cursor *cursor_tmp;
	mongoclient *link;
	char *cmd_ns;

	if (!php_mongo_db_is_valid_dbname(dbname, dbname_len TSRMLS_CC)) {
		return NULL;
	}

	link = (mongoclient*)zend_object_store_get_object(zmongoclient TSRMLS_CC);

	/* create db.$cmd */
	MAKE_STD_ZVAL(ns);
	cmd_ns = get_cmd_ns(dbname, dbname_len);
	ZVAL_STRING(ns, cmd_ns, 0);

	/* create cursor, with RP inherited from us */
	MAKE_STD_ZVAL(cursor);
	object_init_ex(cursor, mongo_ce_Cursor);
	cursor_tmp = (mongo_cursor*)zend_object_store_get_object(cursor TSRMLS_CC);
	mongo_read_preference_replace(read_preferences, &cursor_tmp->read_pref);

	php_mongocursor_create(cursor_tmp, zmongoclient, Z_STRVAL_P(ns), Z_STRLEN_P(ns), cmd, NULL TSRMLS_CC);

	zval_ptr_dtor(&ns);

	MAKE_STD_ZVAL(temp);
	ZVAL_NULL(temp);

	/* limit: all commands need to have set a limit of -1 */
	php_mongo_cursor_set_limit(cursor_tmp, -1);

	/* Mark as a command cursor. If done, this triggers special BSON conversion
	 * to make sure that the cursor ID is represented as MongoInt64. */
	php_mongo_cursor_force_command_cursor(cursor_tmp);

	zval_ptr_dtor(&temp);

	if (options) {
		zval **timeout;

		if (zend_hash_find(HASH_P(options), "socketTimeoutMS", strlen("socketTimeoutMS") + 1, (void**)&timeout) == SUCCESS) {
			convert_to_long(*timeout);
			cursor_tmp->timeout = Z_LVAL_PP(timeout);
		} else if (zend_hash_find(HASH_P(options), "timeout", strlen("timeout") + 1, (void**)&timeout) == SUCCESS) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'timeout' option is deprecated, please use 'socketTimeoutMS' instead");

			convert_to_long(*timeout);
			cursor_tmp->timeout = Z_LVAL_PP(timeout);
		}
	}

	/* Make sure commands are sent to primaries if supported, but only if we
	 * have a replica set connection. */
	/* This should be refactored alongside with the getLastError redirection in
	 * collection.c/append_getlasterror. The Cursor creation should be done
	 * through an init method. */
	if (!link->servers) {
		zend_throw_exception(mongo_ce_Exception, "The MongoClient object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
		return NULL;
	}
	if (php_mongo_command_supports_rp(cmd)) {
		mongo_manager_log(link->manager, MLOG_CON, MLOG_INFO, "command supports Read Preferences");
	} else if (link->servers->options.con_type == MONGO_CON_TYPE_REPLSET) {
		mongo_manager_log(link->manager, MLOG_CON, MLOG_INFO, "forcing primary for command");
		php_mongo_cursor_force_primary(cursor_tmp);
	}

	/* query */
	php_mongo_runquery(cursor_tmp TSRMLS_CC);
	if (EG(exception)) {
		zval_ptr_dtor(&cursor);
		return NULL;
	}

	/* Find return value */
	if (php_mongocursor_load_current_element(cursor_tmp TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&cursor);
		return NULL;
	}

	if (php_mongo_handle_error(cursor_tmp TSRMLS_CC)) {
		/* do not free anything here, as php_mongo_handle_error already does
		 * that upon error */
		return NULL;
	}

	if (!php_mongocursor_is_valid(cursor_tmp)) {
		zval_ptr_dtor(&cursor);
		return NULL;
	}
	MAKE_STD_ZVAL(retval);
	ZVAL_ZVAL(retval, cursor_tmp->current, 1, 0);


	/* Before we destroy the cursor, we figure out which connection was used.
	 * Yes, this is quite ugly but necessary for cursor commands. */
	if (used_connection) {
		*used_connection = cursor_tmp->connection;
	}

	zend_objects_store_del_ref(cursor TSRMLS_CC);
	zval_ptr_dtor(&cursor);

	return retval;
}
/* }}} */

zval* mongo_db__create_fake_cursor(mongo_connection *connection, char *database, zval *cmd TSRMLS_DC)
{
	zval *cursor_zval;
	mongo_cursor *cursor;
	smart_str ns = { NULL, 0, 0 };

	MAKE_STD_ZVAL(cursor_zval);
	object_init_ex(cursor_zval, mongo_ce_Cursor);

	cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);

	cursor->query = cmd;
	zval_add_ref(&cmd);

	if (database) {
		smart_str_append(&ns, database);
		smart_str_appendl(&ns, ".$cmd", 5);
		smart_str_0(&ns);
		cursor->ns = ns.c;
	} else {
		cursor->ns = estrdup("admin.$cmd");
	}

	cursor->fields = 0;
	cursor->limit = -1;
	cursor->skip = 0;
	cursor->opts = 0;
	cursor->current = 0;
	cursor->timeout = PHP_MONGO_STATIC_CURSOR_TIMEOUT_NOT_SET_INITIALIZER;

	php_mongo_cursor_force_command_cursor(cursor);

	return cursor_zval;
}


PHP_METHOD(MongoDB, authenticate)
{
	mongo_db   *db;
	mongoclient *link;
	char       *username, *password;
	int         ulen, plen, i;
	char       *error_message;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &username, &ulen, &password, &plen) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());
	PHP_MONGO_GET_LINK(db->link);

	/* First we check whether the link already has database/username/password
	 * set. If so, we can't re-authenticate and bailout. */
	if (
		link->servers->server[0]->db ||
		link->servers->server[0]->username ||
		link->servers->server[0]->password
	) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "You can't authenticate an already authenticated connection.");
		RETURN_FALSE;
	}

	/* Update all the servers */
	for (i = 0; i < link->servers->count; i++) {
		link->servers->server[i]->db = strdup(Z_STRVAL_P(db->name));
		link->servers->server[i]->authdb = strdup(Z_STRVAL_P(db->name));
		link->servers->server[i]->username = strdup(username);
		link->servers->server[i]->password = strdup(password);
	}

	/* Try to authenticate with the newly set credentials, and fake return
	 * values to be backwards compatible with previous driver versions. */
	array_init(return_value);
	if (mongo_get_read_write_connection(link->manager, link->servers, MONGO_CON_FLAG_READ, (char**) &error_message)) {
		add_assoc_long(return_value, "ok", 1);
	} else {
		add_assoc_long(return_value, "ok", 0);
		add_assoc_string(return_value, "errmsg", error_message, 1);

		/* Reset the credentials since it failed */
		for (i = 0; i < link->servers->count; i++) {
			free(link->servers->server[i]->db);
			link->servers->server[i]->db = NULL;
			free(link->servers->server[i]->authdb);
			link->servers->server[i]->authdb = NULL;
			free(link->servers->server[i]->username);
			link->servers->server[i]->username = NULL;
			free(link->servers->server[i]->password);
			link->servers->server[i]->password = NULL;
		}
		free(error_message);
	}
}

static void clear_exception(zval* return_value TSRMLS_DC)
{
	if (EG(exception)) {
		zval *e, *doc;

		e = EG(exception);
		doc = zend_read_property(mongo_ce_CursorException, e, "doc", strlen("doc"), QUIET TSRMLS_CC);

		if (doc && Z_TYPE_P(doc) == IS_ARRAY && !zend_hash_exists(Z_ARRVAL_P(doc), "$err", strlen("$err") + 1)) {
			RETVAL_ZVAL(doc, 1, 0);
			zend_clear_exception(TSRMLS_C);
		}
	}
}


static void run_err(char *cmd, zval *return_value, zval *dbobj TSRMLS_DC)
{
	zval *command, *retval;
	mongo_db *db;

	MAKE_STD_ZVAL(command);
	array_init(command);
	add_assoc_long(command, cmd, 1);

	PHP_MONGO_GET_DB(dbobj);
	retval = php_mongo_runcommand(db->link, &db->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), command, NULL, 0, NULL TSRMLS_CC);
	clear_exception(return_value TSRMLS_CC);

	zval_ptr_dtor(&command);
	if (retval) {
		RETVAL_ZVAL(retval, 0, 1);
	} else {
		RETVAL_NULL();
	}
}

/* {{{ MongoDB->lastError()
 */
PHP_METHOD(MongoDB, lastError)
{
	run_err("getlasterror", return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ MongoDB->prevError()
 */
PHP_METHOD(MongoDB, prevError)
{
	run_err("getpreverror", return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ MongoDB->resetError()
 */
PHP_METHOD(MongoDB, resetError)
{
	run_err("reseterror", return_value, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ MongoDB->forceError()
 */
PHP_METHOD(MongoDB, forceError)
{
	run_err("forceerror", return_value, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ proto MongoCollection MongoDB::__get(string name)
   Returns the "name" collection from the database */
PHP_METHOD(MongoDB, __get)
{
	char *name;
	int   name_len;
	zval *collection;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		return;
	}

	/* select this collection */
	collection = php_mongo_db_selectcollection(getThis(), name, name_len TSRMLS_CC);
	if (collection) {
		/* Only copy the zval into return_value if it worked. If collection is
		 * NULL here, an exception is set */
		RETVAL_ZVAL(collection, 0, 1);
	}
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(0, connection, MongoClient, 0)
	ZEND_ARG_INFO(0, database_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getGridFS, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, prefix)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setSlaveOkay, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, slave_okay)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setReadPreference, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, read_preference)
	ZEND_ARG_ARRAY_INFO(0, tags, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getWriteConcern, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setWriteConcern, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, w)
	ZEND_ARG_INFO(0, wtimeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setProfilingLevel, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_repair, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, keep_cloned_files)
	ZEND_ARG_INFO(0, backup_original_files)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_selectCollection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, collection_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_createCollection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, collection_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_dropCollection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, collection_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_createDBRef, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, collection_name)
	ZEND_ARG_INFO(0, array_with_id_fields_OR_MongoID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getDBRef, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, reference_information)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_execute, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, javascript_code)
	ZEND_ARG_ARRAY_INFO(0, arguments, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_command, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, command)
	ZEND_ARG_ARRAY_INFO(0, options, 1)
	ZEND_ARG_INFO(1, hash)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_authenticate, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, username)
	ZEND_ARG_INFO(0, password)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_systemCollections, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, includeSystemCollections)
ZEND_END_ARG_INFO()


static zend_function_entry MongoDB_methods[] = {
	PHP_ME(MongoDB, __construct, arginfo___construct, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, __toString, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, __get, arginfo___get, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, getGridFS, arginfo_getGridFS, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, getSlaveOkay, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoDB, setSlaveOkay, arginfo_setSlaveOkay, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoDB, getReadPreference, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, setReadPreference, arginfo_setReadPreference, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, getWriteConcern, arginfo_getWriteConcern, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, setWriteConcern, arginfo_setWriteConcern, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, getProfilingLevel, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, setProfilingLevel, arginfo_setProfilingLevel, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, drop, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, repair, arginfo_repair, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, selectCollection, arginfo_selectCollection, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, createCollection, arginfo_createCollection, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, dropCollection, arginfo_dropCollection, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, listCollections, arginfo_systemCollections, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, getCollectionNames, arginfo_systemCollections, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, getCollectionInfo, arginfo_systemCollections, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, createDBRef, arginfo_createDBRef, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, getDBRef, arginfo_getDBRef, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, execute, arginfo_execute, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoDB, command, arginfo_command, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, lastError, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDB, prevError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoDB, resetError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoDB, forceError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoDB, authenticate, arginfo_authenticate, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_FE_END
};

static void php_mongo_db_free(void *object TSRMLS_DC)
{
	mongo_db *db = (mongo_db*)object;

	if (db) {
		if (db->link) {
			zval_ptr_dtor(&db->link);
		}
		if (db->name) {
			zval_ptr_dtor(&db->name);
		}
		mongo_read_preference_dtor(&db->read_pref);
		zend_object_std_dtor(&db->std TSRMLS_CC);
		efree(db);
	}
}

/* {{{ mongo_mongo_db_new
 */
zend_object_value php_mongo_db_new(zend_class_entry *class_type TSRMLS_DC) {
	PHP_MONGO_OBJ_NEW(mongo_db);
}
/* }}} */

void mongo_init_MongoDB(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoDB", MongoDB_methods);
	ce.create_object = php_mongo_db_new;
	mongo_ce_DB = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_OFF", strlen("PROFILING_OFF"), 0 TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_SLOW", strlen("PROFILING_SLOW"), 1 TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_ON", strlen("PROFILING_ON"), 2 TSRMLS_CC);

	zend_declare_property_long(mongo_ce_DB, "w", strlen("w"), 1, ZEND_ACC_PUBLIC|MONGO_ACC_READ_ONLY TSRMLS_CC);
	zend_declare_property_long(mongo_ce_DB, "wtimeout", strlen("wtimeout"), PHP_MONGO_DEFAULT_WTIMEOUT, ZEND_ACC_PUBLIC|MONGO_ACC_READ_ONLY TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
