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
#include "../php_mongo.h"
#include "../mongoclient.h"
#include "../db.h"
#include "../collection.h"
#include "db_ref.h"

extern zend_class_entry *mongo_ce_DB, *mongo_ce_Id, *mongo_ce_Exception;

zend_class_entry *mongo_ce_DBRef = NULL;

/* {{{ MongoDBRef::create(string collection, mixed id [, string db])
 *
 * DBRefs are of the form:
 * array('$ref' => <collection>, '$id' => <id> [, $db => <db>]) */
PHP_METHOD(MongoDBRef, create)
{
	char *collection, *db = NULL;
	int collection_len, db_len = 0;
	zval *zid, *retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|s", &collection, &collection_len, &zid, &db, &db_len) == FAILURE) {
		return;
	}

	retval = php_mongo_dbref_create(zid, collection, db TSRMLS_CC);

	if (retval) {
		RETURN_ZVAL(retval, 0, 1);
	}

	RETURN_NULL();
}
/* }}} */

zval *php_mongo_dbref_create(zval *zid, char *collection, char *db TSRMLS_DC)
{
	zval *retval;

	MAKE_STD_ZVAL(retval);
	array_init(retval);

	/* add collection name */
	add_assoc_string(retval, "$ref", collection, 1);

	/* add id field */
	add_assoc_zval(retval, "$id", zid);
	zval_add_ref(&zid);

	/* if we got a database name, add that, too */
	if (db) {
		add_assoc_string(retval, "$db", db, 1);
	}

	return retval;
}

zval *php_mongo_dbref_resolve_id(zval *zid TSRMLS_DC)
{
	/* If zid is an array or non-MongoId object, return its "_id" field */
	if (Z_TYPE_P(zid) == IS_ARRAY || (Z_TYPE_P(zid) == IS_OBJECT && !instanceof_function(Z_OBJCE_P(zid), mongo_ce_Id TSRMLS_CC))) {
		zval **tmpval;

		if (zend_hash_find(HASH_P(zid), "_id", 4, (void**)&tmpval) == SUCCESS) {
			zid = *tmpval;
		} else if (Z_TYPE_P(zid) == IS_ARRAY) {
			return NULL;
		}
	}

	return zid;
}

/* {{{ proto bool MongoDBRef::isRef(mixed ref)
   Checks if $ref has a $ref and $id property/key */
PHP_METHOD(MongoDBRef, isRef)
{
	zval *ref;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
		return;
	}

	if (IS_SCALAR_P(ref)) {
		RETURN_FALSE;
	}

	/* check that $ref and $id fields exists */
	if (zend_hash_exists(HASH_P(ref), "$ref", strlen("$ref") + 1) && zend_hash_exists(HASH_P(ref), "$id", strlen("$id") + 1)) {
		/* good enough */
		RETURN_TRUE;
	}

	RETURN_FALSE;
}
/* }}} */
	
void php_mongo_dbref_get(zval *zdb, zval *ref, zval *return_value TSRMLS_DC)
{
	zval *collection, *query;
	zval **ns, **id, **dbname;
	zend_bool alloced_db = 0;
	mongo_db *db;
	PHP_MONGO_GET_DB(zdb);

	if (
		IS_SCALAR_P(ref) ||
		zend_hash_find(HASH_P(ref), "$ref", strlen("$ref") + 1, (void**)&ns) == FAILURE ||
		zend_hash_find(HASH_P(ref), "$id", strlen("$id") + 1, (void**)&id) == FAILURE
	) {
		RETURN_NULL();
	}

	if (Z_TYPE_PP(ns) != IS_STRING) {
		zend_throw_exception(mongo_ce_Exception, "MongoDBRef::get: $ref field must be a string", 10 TSRMLS_CC);
		return;
	}

	/* if this reference contains a db name, we have to switch dbs */
	if (zend_hash_find(HASH_P(ref), "$db", strlen("$db") + 1, (void**)&dbname) == SUCCESS) {
		/* just to be paranoid, make sure dbname is a string */
		if (Z_TYPE_PP(dbname) != IS_STRING) {
			zend_throw_exception(mongo_ce_Exception, "MongoDBRef::get: $db field must be a string", 11 TSRMLS_CC);
			return;
		}

		/* if the name in the $db field doesn't match the current db, make up
		 * a new db */
		if (strcmp(Z_STRVAL_PP(dbname), Z_STRVAL_P(db->name)) != 0) {
			zdb = php_mongoclient_selectdb(db->link, Z_STRVAL_PP(dbname), Z_STRLEN_PP(dbname) TSRMLS_CC);
			if (zdb == NULL) {
				return;
			}

			/* so we can dtor this later */
			alloced_db = 1;
		}
	}

	/* get the collection */
	collection = php_mongo_db_selectcollection(zdb, Z_STRVAL_PP(ns), Z_STRLEN_PP(ns) TSRMLS_CC);
	if (!collection) {
		if (alloced_db) {
			zval_ptr_dtor(&zdb);
		}
		return;
	}

	/* query for the $id */
	MAKE_STD_ZVAL(query);
	array_init(query);
	add_assoc_zval(query, "_id", *id);
	zval_add_ref(id);

	/* return whatever's there */
	php_mongo_collection_findone(collection, query, NULL, NULL, return_value TSRMLS_CC);

	/* cleanup */
	zval_ptr_dtor(&collection);
	zval_ptr_dtor(&query);
	if (alloced_db) {
		zval_ptr_dtor(&zdb);
	}
}

/* {{{ MongoDBRef::get(MongoDB $db, array $ref)
 * Fetches the object pointed to by a reference */
PHP_METHOD(MongoDBRef, get)
{
	zval *zdb, *ref;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &zdb, mongo_ce_DB, &ref) == FAILURE) {
		return;
	}

	php_mongo_dbref_get(zdb, ref, return_value TSRMLS_CC);
}
/* }}} */

static zend_function_entry MongoDBRef_methods[] = {
	PHP_ME(MongoDBRef, create, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(MongoDBRef, isRef, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(MongoDBRef, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};


void mongo_init_MongoDBRef(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoDBRef", MongoDBRef_methods);
	mongo_ce_DBRef = zend_register_internal_class(&ce TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
