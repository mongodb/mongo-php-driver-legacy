/**
 *  Copyright 2014 MongoDB, Inc.
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
#include "../api/write.h"
#include "../api/batch.h"
#include "../batch/write.h"
#include "../batch/write_private.h"
#include "../collection.h" /* mongo_apply_implicit_write_options() */

/* The Batch API is only available for 5.3.0+ */
#if PHP_VERSION_ID >= 50300

ZEND_EXTERN_MODULE_GLOBALS(mongo)

extern zend_class_entry *mongo_ce_Collection;
extern zend_class_entry *mongo_ce_Exception;
extern zend_object_handlers mongo_type_object_handlers;

zend_class_entry *mongo_ce_WriteBatch = NULL;

void php_mongo_write_batch_object_free(void *object TSRMLS_DC) /* {{{ */
{
	mongo_write_batch_object *intern = (mongo_write_batch_object *)object;

	if (intern) {
		/* If the ctor fails then we won't have a MongoCollection object */
		if (intern->zcollection_object) {
			Z_DELREF_P(intern->zcollection_object);
		}

		zend_object_std_dtor(&intern->std TSRMLS_CC);
		efree(intern);
	}
}
/* }}} */

zend_object_value php_mongo_write_batch_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	mongo_write_batch_object *intern;
	php_mongo_write_options write_options = {-1, {-1}, -1, -1, -1, -1};

	intern = (mongo_write_batch_object *)emalloc(sizeof(mongo_write_batch_object));
	memset(intern, 0, sizeof(mongo_write_batch_object));

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	init_properties(intern);

	intern->write_options = write_options;

	retval.handle = zend_objects_store_put(intern,
		(zend_objects_store_dtor_t) zend_objects_destroy_object,
		php_mongo_write_batch_object_free, NULL TSRMLS_CC);
	retval.handlers = &mongo_type_object_handlers;

	return retval;
}
/* }}} */

/* {{{ proto MongoWriteBatch MongoWriteBatch::__construct(MongoCollection $collection, long $batch_type, [array $write_options])
   Constructs a new Write Batch of $batch_type operations */
PHP_METHOD(MongoWriteBatch, __construct)
{
	long  batch_type;
	zend_error_handling error_handling;
	mongo_write_batch_object *intern;
	HashTable *write_options;
	zval *zcollection;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ol|h", &zcollection, mongo_ce_Collection, &batch_type, &write_options) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	switch(batch_type) {
		case MONGODB_API_COMMAND_INSERT:
		case MONGODB_API_COMMAND_UPDATE:
		case MONGODB_API_COMMAND_DELETE:
			break;
		default:
			zend_throw_exception(mongo_ce_Exception, "Invalid argument, must one of the write methods", 1 TSRMLS_CC);
			return;
	}

	php_mongo_api_batch_ctor(intern, zcollection, batch_type, write_options TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool MongoWriteBatch::add(array $item)
   Adds a new item to the batch. Throws MongoException if $item is missing required keys */
PHP_METHOD(MongoWriteBatch, add)
{
	zval *z_item;
	mongo_connection *connection;
	mongo_collection *collection;
	zend_error_handling error_handling;
	mongo_write_batch_object *intern;
	php_mongo_write_item        item;
	php_mongo_write_update_args update_args;
	php_mongo_write_delete_args delete_args;
	int status;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &z_item) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}

	collection = (mongo_collection *)zend_object_store_get_object(intern->zcollection_object TSRMLS_CC);
	connection = get_server(collection, MONGO_CON_FLAG_WRITE TSRMLS_CC);
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	/* If we haven't allocated a batch yet, or need to start a new one */
	if (intern->total_items == 0 || intern->batch->item_count >= connection->max_write_batch_size) {
		php_mongo_api_batch_make_from_collection_object(intern, intern->zcollection_object, intern->batch_type TSRMLS_CC);
	}

	item.type = intern->batch_type;
	switch(intern->batch_type) {
		case MONGODB_API_COMMAND_INSERT:
			item.write.insert = Z_ARRVAL_P(z_item);
			break;

		case MONGODB_API_COMMAND_UPDATE: {
			zval **q, **u, **multi, **upsert;

			if (FAILURE == zend_hash_find(Z_ARRVAL_P(z_item), "q", strlen("q") + 1, (void**) &q)) {
				zend_throw_exception(mongo_ce_Exception, "Expected $item to contain 'q' key", 0 TSRMLS_CC);
				return;
			}

			if (FAILURE == zend_hash_find(Z_ARRVAL_P(z_item), "u", strlen("u") + 1, (void**) &u)) {
				zend_throw_exception(mongo_ce_Exception, "Expected $item to contain 'u' key", 0 TSRMLS_CC);
				return;
			}

			convert_to_array_ex(q);
			convert_to_array_ex(u);
			update_args.query = *q;
			update_args.update = *u;

			if (SUCCESS == zend_hash_find(Z_ARRVAL_P(z_item), "multi", strlen("multi") + 1, (void**) &multi)) {
				convert_to_boolean_ex(multi);
				update_args.multi = Z_BVAL_PP(multi);
			}

			if (SUCCESS == zend_hash_find(Z_ARRVAL_P(z_item), "upsert", strlen("upsert") + 1, (void**) &upsert)) {
				convert_to_boolean_ex(upsert);
				update_args.upsert = Z_BVAL_PP(upsert);
			}

			item.write.update = &update_args;
			break;
		 }

		case MONGODB_API_COMMAND_DELETE: {
			zval **q, **limit;

			if (FAILURE == zend_hash_find(Z_ARRVAL_P(z_item), "q", strlen("q") + 1, (void**) &q)) {
				zend_throw_exception(mongo_ce_Exception, "Expected $item to contain 'q' key", 0 TSRMLS_CC);
				return;
			}

			convert_to_array_ex(q);
			delete_args.query = *q;

			if (SUCCESS == zend_hash_find(Z_ARRVAL_P(z_item), "limit", strlen("limit") + 1, (void**) &limit)) {
				convert_to_long_ex(limit);
				delete_args.limit = Z_LVAL_PP(limit);
				return;
			}

			item.write.delete = &delete_args;
			break;
		 }

		default:
			RETURN_FALSE;
	}

	status = php_mongo_api_write_add(&intern->batch->buffer, intern->batch->item_count++, &item, connection->max_bson_size TSRMLS_CC);

	if (status == FAILURE) {
		/* exception thrown */
		RETURN_FALSE;
	}

	if (status == SUCCESS) {
		intern->total_items++;
		RETURN_TRUE;
	}

	/* It is in a limbo. It didn't fail, but it did overflow the buffer. */
	intern->batch->item_count--;
	php_mongo_api_batch_make_from_collection_object(intern, intern->zcollection_object, intern->batch_type TSRMLS_CC);

	status = php_mongo_api_write_add(&intern->batch->buffer, intern->batch->item_count++, &item, connection->max_bson_size TSRMLS_CC);

	if (status == SUCCESS) {
		intern->total_items++;
		RETURN_TRUE;
	}
	RETURN_FALSE;

}
/* }}} */

/* {{{ proto array MongoWriteBatch::execute(array $write_options)
   Executes the constructed batch. Returns the server response */
PHP_METHOD(MongoWriteBatch, execute)
{
	HashTable *write_options;
	zend_error_handling error_handling;
	mongo_write_batch_object *intern;
	mongo_collection *collection;
	mongo_connection *connection;
	mongoclient      *link;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "h", &write_options) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}

	zend_restore_error_handling(&error_handling TSRMLS_CC);
	if (!intern->total_items) {
		zend_throw_exception(mongo_ce_Exception, "No items in batch", 1 TSRMLS_CC);
		return;
	}

	collection = (mongo_collection*)zend_object_store_get_object(intern->zcollection_object TSRMLS_CC);

	link       = (mongoclient *)zend_object_store_get_object(collection->link TSRMLS_CC);
	connection = get_server(collection, MONGO_CON_FLAG_WRITE TSRMLS_CC);

	/* Reset the item counter */
	intern->total_items = 0;

	php_mongo_api_write_options_from_ht(&intern->write_options, write_options TSRMLS_CC);

	intern->batch = intern->batch->first;
	array_init(return_value);
	do {
		php_mongo_batch *batch = intern->batch;
		int status = php_mongo_api_batch_execute(batch, &intern->write_options, connection, &link->servers->options, return_value TSRMLS_CC);

		if (status) {
			if (status == 1) {
				zval_dtor(return_value);
				php_mongo_api_batch_free(batch);
			} else {
				efree(batch);
			}
			break;
		}

		intern->batch = batch->next;
		efree(batch);
	} while(intern->batch);
}
/* }}} */

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(0, collection, MongoCollection, 0)
	ZEND_ARG_INFO(0, batch_type)
	ZEND_ARG_ARRAY_INFO(0, write_options, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_add, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, item, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_execute, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, write_options, 0)
ZEND_END_ARG_INFO()

static zend_function_entry MongoWriteBatch_methods[] = {
	PHP_ME(MongoWriteBatch, __construct, arginfo___construct, ZEND_ACC_PROTECTED)
	PHP_ME(MongoWriteBatch, add, arginfo_add, ZEND_ACC_PUBLIC)
	PHP_ME(MongoWriteBatch, execute, arginfo_execute, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_FE_END
};

void mongo_init_MongoWriteBatch(TSRMLS_D)
{
	zend_class_entry write_batch;
	INIT_CLASS_ENTRY(write_batch, "MongoWriteBatch", MongoWriteBatch_methods);

	write_batch.create_object = php_mongo_write_batch_object_new;

	mongo_ce_WriteBatch = zend_register_internal_class(&write_batch TSRMLS_CC);
}

#endif /* PHP_VERSION_ID >= 50300 */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
