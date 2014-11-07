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
#include <ext/standard/php_array.h>
#include "../php_mongo.h"
#include "../api/write.h"
#include "../api/batch.h"
#include "../batch/write.h"
#include "../batch/write_private.h"
#include "../collection.h"
#include "../log_stream.h"
#include "../mcon/manager.h"

ZEND_EXTERN_MODULE_GLOBALS(mongo)

extern zend_class_entry *mongo_ce_Collection;
extern zend_class_entry *mongo_ce_Exception;
extern zend_class_entry *mongo_ce_WriteConcernException;
extern zend_object_handlers mongo_type_object_handlers;

zend_class_entry *mongo_ce_WriteBatch = NULL;

/* frees an mongo_write_batch_object allocated by php_mongo_write_batch_object_new
 * This is an internal method to PHP and shall not be used explicitly */
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

/* Allocates a new mongo_write_batch_object.
 * This is an internal method to PHP and shall not be used explicitly */
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
	HashTable *write_options = NULL;
	zval *zcollection;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ol|h", &zcollection, mongo_ce_Collection, &batch_type, &write_options) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	switch (batch_type) {
		case MONGODB_API_COMMAND_INSERT:
		case MONGODB_API_COMMAND_UPDATE:
		case MONGODB_API_COMMAND_DELETE:
			break;

		default:
			zend_throw_exception_ex(mongo_ce_Exception, 1 TSRMLS_CC, "Invalid batch type specified: %ld", batch_type);
			return;
	}

	php_mongo_api_batch_ctor(intern, zcollection, batch_type, write_options TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool MongoWriteBatch::add(array $item)
   Adds a new item to the batch. Throws MongoException if $item is missing required keys */
PHP_METHOD(MongoWriteBatch, add)
{
	HashTable *ht_item;
	mongo_connection *connection;
	mongo_collection *collection;
	mongo_write_batch_object *intern;
	php_mongo_write_item write_item;
	php_mongo_write_update_args update_args = { NULL, NULL, -1, -1 };
	php_mongo_write_delete_args delete_args = { NULL, -1 };
	int status;
	mongoclient *link;

	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(intern->zcollection_object, MongoWriteBatch);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "H", &ht_item) == FAILURE) {
		return;
	}

	collection = (mongo_collection *)zend_object_store_get_object(intern->zcollection_object TSRMLS_CC);

	link       = (mongoclient *)zend_object_store_get_object(collection->link TSRMLS_CC);
	connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC);
	if (!connection) {
		/* Exception thrown by php_mongo_collection_get_server() */
		return;
	}

	/* If we haven't allocated a batch yet, or need to start a new one */
	if (intern->total_items == 0 || intern->batch->item_count >= connection->max_write_batch_size) {
		php_mongo_api_batch_make_from_collection_object(intern, intern->zcollection_object, intern->batch_type TSRMLS_CC);
	}

	write_item.type = intern->batch_type;
	switch (intern->batch_type) {
		case MONGODB_API_COMMAND_INSERT:
			write_item.write.insert_doc = ht_item;
			break;

		case MONGODB_API_COMMAND_UPDATE: {
			zval **q, **u, **multi, **upsert;

			if (FAILURE == zend_hash_find(ht_item, "q", strlen("q") + 1, (void**) &q)) {
				zend_throw_exception(mongo_ce_Exception, "Expected $item to contain 'q' key", 0 TSRMLS_CC);
				return;
			}

			if (FAILURE == zend_hash_find(ht_item, "u", strlen("u") + 1, (void**) &u)) {
				zend_throw_exception(mongo_ce_Exception, "Expected $item to contain 'u' key", 0 TSRMLS_CC);
				return;
			}

			convert_to_array_ex(q);
			convert_to_array_ex(u);
			update_args.query = *q;
			update_args.update = *u;

			if (SUCCESS == zend_hash_find(ht_item, "multi", strlen("multi") + 1, (void**) &multi)) {
				convert_to_boolean_ex(multi);
				update_args.multi = Z_BVAL_PP(multi);
			}

			if (SUCCESS == zend_hash_find(ht_item, "upsert", strlen("upsert") + 1, (void**) &upsert)) {
				convert_to_boolean_ex(upsert);
				update_args.upsert = Z_BVAL_PP(upsert);
			}

			write_item.write.update_args = &update_args;
			break;
		 }

		case MONGODB_API_COMMAND_DELETE: {
			zval **q, **limit;

			if (FAILURE == zend_hash_find(ht_item, "q", strlen("q") + 1, (void**) &q)) {
				zend_throw_exception(mongo_ce_Exception, "Expected $item to contain 'q' key", 0 TSRMLS_CC);
				return;
			}

			if (FAILURE == zend_hash_find(ht_item, "limit", strlen("limit") + 1, (void**) &limit)) {
				zend_throw_exception(mongo_ce_Exception, "Expected $item to contain 'limit' key", 0 TSRMLS_CC);
				return;
			}

			convert_to_array_ex(q);
			convert_to_long_ex(limit);
			delete_args.query = *q;
			delete_args.limit = Z_LVAL_PP(limit);

			write_item.write.delete_args = &delete_args;
			break;
		 }

		default:
			RETURN_FALSE;
	}

	status = php_mongo_api_write_add(&intern->batch->buffer, intern->batch->item_count++, &write_item, connection->max_bson_size TSRMLS_CC);

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

	status = php_mongo_api_write_add(&intern->batch->buffer, intern->batch->item_count++, &write_item, connection->max_bson_size TSRMLS_CC);

	if (status == SUCCESS) {
		intern->total_items++;
		RETURN_TRUE;
	}
	RETURN_FALSE;

}
/* }}} */

int php_mongo_api_return_value_get_int_del(zval *data, char *key)
{
	zval **val;

	if (zend_hash_find(Z_ARRVAL_P(data), key, strlen(key) + 1, (void**)&val) == SUCCESS) {
		int retval = 0;
		convert_to_long_ex(val);
		retval = Z_LVAL_PP(val);

		zend_hash_del_key_or_index(Z_ARRVAL_P(data), key, strlen(key) + 1, 0, HASH_DEL_KEY);
		return retval;
	}

	return 0;
}

/* Executes the constructed MongoWriteBatch (Mongo[Insert|Update|Delete]Batch) */
void php_mongo_writebatch_execute(mongo_write_batch_object *intern, mongo_connection *connection, mongoclient *link, zval *return_value TSRMLS_DC) /* {{{ */
{
	php_mongo_batch *first = intern->batch->first;
	int ok = 0, n = 0, nModified = 0, nUpserted = 0;
	int status;

	do {
		zval *batch_retval;
		php_mongo_batch *batch = intern->batch;
		zval **errors;
		zval **upserted;

		MAKE_STD_ZVAL(batch_retval);
		array_init(batch_retval);
		status = php_mongo_api_batch_execute(batch, &intern->write_options, connection, &link->servers->options, batch_retval TSRMLS_CC);

		mongo_log_stream_write_batch(connection, &intern->write_options, batch->request_id, batch_retval TSRMLS_CC);

		if (status) {
			zval_ptr_dtor(&batch_retval);
			break;
		}

		/* writeErrors = continue when ordered =false */
		if (zend_hash_find(Z_ARRVAL_P(batch_retval), "writeErrors", strlen("writeErrors") + 1, (void**)&errors) == SUCCESS) {
			HashPosition pointer;
			zval **data;
			char *key;
			uint index_key_len;
			ulong uindex;
			HashTable *hindex = Z_ARRVAL_PP(errors);

			for (
				zend_hash_internal_pointer_reset_ex(hindex, &pointer);
				zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS;
				zend_hash_move_forward_ex(hindex, &pointer)
			) {
				uint key_type = zend_hash_get_current_key_ex(hindex, &key, &index_key_len, &uindex, NO_DUP, &pointer);
				zval **zindex;

				if (key_type != HASH_KEY_IS_LONG) {
					continue;
				}
				if (zend_hash_find(Z_ARRVAL_PP(data), "index", strlen("index")+1, (void **)&zindex) == SUCCESS) {
					convert_to_long(*zindex);
					Z_LVAL_PP(zindex) += n;
				}
			}
			if (intern->write_options.ordered) {
				status = 1;
			}
		}

		/* Always continue on writeConcernErrors, no matter ordered=true/false.
		 * No need to do anything special there as we already array_merge() the batch_retval, and there is no
		 * index rewrite needed */

		if (zend_hash_find(Z_ARRVAL_P(batch_retval), "upserted", strlen("upserted") + 1, (void**)&upserted) == SUCCESS) {
			HashPosition pointer;
			zval **data;
			char *key;
			uint index_key_len;
			ulong uindex;
			HashTable *hindex = Z_ARRVAL_PP(upserted);

			for (
				zend_hash_internal_pointer_reset_ex(hindex, &pointer);
				zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS;
				zend_hash_move_forward_ex(hindex, &pointer)
			) {
				uint key_type = zend_hash_get_current_key_ex(hindex, &key, &index_key_len, &uindex, NO_DUP, &pointer);
				zval **zindex;

				if (key_type != HASH_KEY_IS_LONG) {
					continue;
				}
				if (zend_hash_find(Z_ARRVAL_PP(data), "index", strlen("index")+1, (void **)&zindex) == SUCCESS) {
					convert_to_long(*zindex);
					Z_LVAL_PP(zindex) += n;
				}
			}
			nUpserted += zend_hash_num_elements(Z_ARRVAL_PP(upserted));
		}

		n += php_mongo_api_return_value_get_int_del(batch_retval, "n");
		/* As long as we have one ok=true then we return ok=true in our container */
		ok += php_mongo_api_return_value_get_int_del(batch_retval, "ok");
		/* Only available for updates though, but it has messed up logic */
		nModified += php_mongo_api_return_value_get_int_del(batch_retval, "nModified");

		zend_hash_del_key_or_index(Z_ARRVAL_P(batch_retval), "ok", strlen("ok") + 1, 0, HASH_DEL_KEY);

		php_array_merge(Z_ARRVAL_P(return_value), Z_ARRVAL_P(batch_retval), 1 TSRMLS_CC);

		intern->batch = intern->batch->next;
		zval_ptr_dtor(&batch_retval);
	} while (intern->batch && status == 0);

	php_mongo_api_batch_free(first);

	/* Bad things happened to the socket when reading/writing it */
	if (status == 2) {
		mongo_manager_connection_deregister(MonGlo(manager), connection);
	}

	switch (intern->batch_type) {
		case MONGODB_API_COMMAND_INSERT:
			add_assoc_long(return_value, "nInserted", n);
			break;

		case MONGODB_API_COMMAND_DELETE:
			add_assoc_long(return_value, "nRemoved", n);
			break;

		case MONGODB_API_COMMAND_UPDATE:
			add_assoc_long(return_value, "nMatched", n-nUpserted);
			add_assoc_long(return_value, "nModified", nModified);
			add_assoc_long(return_value, "nUpserted", nUpserted);
			break;
	}

	add_assoc_bool(return_value, "ok", ok);
}
/* }}} */

/* {{{ proto array MongoWriteBatch::execute([array $write_options])
   Executes the constructed batch. Returns the server response */
PHP_METHOD(MongoWriteBatch, execute)
{
	HashTable *write_options = NULL;
	mongo_write_batch_object *intern;
	mongo_collection *collection;
	mongo_connection *connection;
	mongoclient      *link;
	zval **errors;

	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(intern->zcollection_object, MongoWriteBatch);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|h", &write_options) == FAILURE) {
		return;
	}

	if (!intern->total_items) {
		/* Emulate the exception msg and code thrown by the server had this been a round-trip */
		/* SEE PHP-1019 */
		zend_throw_exception(mongo_ce_Exception, "No write ops were included in the batch", 16 TSRMLS_CC);
		return;
	}

	collection = (mongo_collection*)zend_object_store_get_object(intern->zcollection_object TSRMLS_CC);

	link       = (mongoclient *)zend_object_store_get_object(collection->link TSRMLS_CC);
	connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC);
	if (!connection) {
		/* Exception thrown by php_mongo_collection_get_server() */
		return;
	}

	/* Reset the item counter */
	intern->total_items = 0;

	if (write_options) {
		php_mongo_api_write_options_from_ht(&intern->write_options, write_options TSRMLS_CC);
	}

	array_init(return_value);
	intern->batch = intern->batch->first;
	php_mongo_writebatch_execute(intern, connection, link, return_value TSRMLS_CC);

	if (zend_hash_find(Z_ARRVAL_P(return_value), "writeErrors", strlen("writeErrors") + 1, (void**)&errors) == SUCCESS) {
		zval *e = zend_throw_exception(mongo_ce_WriteConcernException, "Failed write", 911 TSRMLS_CC);
		zend_update_property(mongo_ce_WriteConcernException, e, "document", strlen("document"), return_value TSRMLS_CC);
	} else if (zend_hash_find(Z_ARRVAL_P(return_value), "writeConcernError", strlen("writeConcernError") + 1, (void**)&errors) == SUCCESS) {
		zval *e = zend_throw_exception(mongo_ce_WriteConcernException, "Failed write", 911 TSRMLS_CC);
		zend_update_property(mongo_ce_WriteConcernException, e, "document", strlen("document"), return_value TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto array MongoWriteBatch::getItemCount()
   Returns how many items have been added so far */
PHP_METHOD(MongoWriteBatch, getItemCount)
{
	mongo_write_batch_object *intern;

	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(intern->zcollection_object, MongoWriteBatch);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_LONG(intern->total_items);
}
/* }}} */

/* {{{ proto array MongoWriteBatch::getBatchInfo()
   Returns how many items are in individual batch and its size (in bytes) */
PHP_METHOD(MongoWriteBatch, getBatchInfo)
{
	mongo_write_batch_object *intern;
	php_mongo_batch *batch;

	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(intern->zcollection_object, MongoWriteBatch);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(return_value);
	/* Either been cleared or never anything added */
	if (!intern->total_items) {
		return;
	}

	batch = intern->batch->first;
	do {
		zval *info;
		ALLOC_ZVAL(info);
		array_init(info);
		INIT_PZVAL(info);

		add_assoc_long(info, "count", batch->item_count);
		add_assoc_long(info, "size", batch->buffer.pos - batch->buffer.start);
		zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &info, sizeof(zval *), NULL);
		
		batch = batch->next;
	} while(batch);
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(0, collection, MongoCollection, 0)
	ZEND_ARG_INFO(0, batch_type)
	ZEND_ARG_ARRAY_INFO(0, write_options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_add, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, item)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_execute, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_ARRAY_INFO(0, write_options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getitemcount, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getbatchinfo, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry MongoWriteBatch_methods[] = {
	PHP_ME(MongoWriteBatch, __construct, arginfo___construct, ZEND_ACC_PROTECTED)
	PHP_ME(MongoWriteBatch, add, arginfo_add, ZEND_ACC_PUBLIC)
	PHP_ME(MongoWriteBatch, execute, arginfo_execute, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(MongoWriteBatch, getItemCount, arginfo_getitemcount, ZEND_ACC_PUBLIC)
	PHP_ME(MongoWriteBatch, getBatchInfo, arginfo_getbatchinfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void mongo_init_MongoWriteBatch(TSRMLS_D)
{
	zend_class_entry write_batch;
	INIT_CLASS_ENTRY(write_batch, "MongoWriteBatch", MongoWriteBatch_methods);

	write_batch.create_object = php_mongo_write_batch_object_new;

	mongo_ce_WriteBatch = zend_register_internal_class(&write_batch TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_WriteBatch, "COMMAND_INSERT", strlen("COMMAND_INSERT"), MONGODB_API_COMMAND_INSERT TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_WriteBatch, "COMMAND_UPDATE", strlen("COMMAND_UPDATE"), MONGODB_API_COMMAND_UPDATE TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_WriteBatch, "COMMAND_DELETE", strlen("COMMAND_DELETE"), MONGODB_API_COMMAND_DELETE TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
