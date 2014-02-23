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
#include "../batch/write.h"
#include "../collection.h" /* mongo_apply_implicit_write_options() */

/* The Batch API is only available for 5.3.0+ */
#if PHP_VERSION_ID >= 50300

ZEND_EXTERN_MODULE_GLOBALS(mongo)

extern zend_class_entry *mongo_ce_Collection;
extern zend_class_entry *mongo_ce_Exception;
extern zend_object_handlers mongo_type_object_handlers;

zend_class_entry *mongo_ce_WriteBatch = NULL;

void php_mongo_write_batch_object_free(void *object TSRMLS_DC)
{
	mongo_write_batch_object *intern = (mongo_write_batch_object *)object;

	if (intern) {
		/* If the ctor fails then we won't have a MongoCollection object */
		if (intern->zcollection_object) {
			Z_DELREF_P(intern->zcollection_object);
		}

		/* We only need to clean the buffer if we never executed it */
		if (intern->request_id) {
			efree(intern->buf.start);
		}

		zend_object_std_dtor(&intern->std TSRMLS_CC);
		efree(intern);
	}
}

zend_object_value php_mongo_write_batch_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	mongo_write_batch_object *intern;

	intern = (mongo_write_batch_object *)emalloc(sizeof(mongo_write_batch_object));
	memset(intern, 0, sizeof(mongo_write_batch_object));

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	init_properties(intern);

	retval.handle = zend_objects_store_put(intern,
		(zend_objects_store_dtor_t) zend_objects_destroy_object,
		php_mongo_write_batch_object_free, NULL TSRMLS_CC);
	retval.handlers = &mongo_type_object_handlers;

	return retval;
}

void php_mongo_write_batch_ctor(mongo_write_batch_object *intern, zval *zcollection, php_mongodb_write_types type TSRMLS_DC)
{
	mongo_db *db;
	char *cmd_ns;
	mongo_collection *collection;

	intern->zcollection_object = zcollection;
	Z_ADDREF_P(zcollection);

	collection = (mongo_collection*)zend_object_store_get_object(zcollection TSRMLS_CC);
	db = (mongo_db*)zend_object_store_get_object(collection->parent TSRMLS_CC);

	CREATE_BUF(intern->buf, INITIAL_BUF_SIZE);
	intern->request_id = MonGlo(request_id);

	spprintf(&cmd_ns, 0, "%s.$cmd", Z_STRVAL_P(db->name));
	intern->container_pos = php_mongo_api_write_header(&intern->buf, cmd_ns TSRMLS_CC);
	intern->batch_pos     = php_mongo_api_write_start(&intern->buf, type, Z_STRVAL_P(collection->name) TSRMLS_CC);
	efree(cmd_ns);
}

int php_mongo_batch_finalize(mongo_buffer *buf, int container_pos, int batch_pos, zval *zcollection_object, HashTable *write_concern TSRMLS_DC)
{
	php_mongodb_write_options write_options = {-1, {-1}, -1, -1, -1, -1};
	int message_length;
	mongoclient      *link;
	mongo_connection *connection;
	mongo_collection *collection;

	collection = (mongo_collection *)zend_object_store_get_object(zcollection_object TSRMLS_CC);
	link       = (mongoclient *)zend_object_store_get_object(collection->link TSRMLS_CC);

	mongo_apply_implicit_write_options(&write_options, &link->servers->options, zcollection_object TSRMLS_CC);
	php_mongo_api_write_options_from_ht(&write_options, write_concern TSRMLS_CC);

	connection = get_server(collection, MONGO_CON_FLAG_WRITE TSRMLS_CC);
	message_length = php_mongo_api_write_end(buf, container_pos, batch_pos, MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size), &write_options TSRMLS_CC);

	/* The document is greater then allowed limits, exception already thrown */
	if (message_length == 0) {
		efree(buf->start);
	}
	return message_length;
}

int php_mongo_batch_send_and_read(mongo_buffer *buf, int request_id, zval *zcollection_object, zval *return_value TSRMLS_DC)
{
	mongo_connection *connection;
	mongo_collection *collection;
	mongoclient      *link;
	int               bytes_written;
	char             *error_message;
	int               success;

	collection = (mongo_collection *)zend_object_store_get_object(zcollection_object TSRMLS_CC);
	link       = (mongoclient *)zend_object_store_get_object(collection->link TSRMLS_CC);
	connection = get_server(collection, MONGO_CON_FLAG_WRITE TSRMLS_CC);

	if (connection == 0) {
		return 0;
	}

	bytes_written = MonGlo(manager)->send(connection, &link->servers->options, buf->start, buf->pos - buf->start, &error_message);
	if (bytes_written < 1) {
		/* Didn't write anything, something bad must have happened */
		efree(buf->start);
		return 0;
	}

	success = php_mongo_api_get_reply(MonGlo(manager), connection, &link->servers->options, 0 /*socket_read_timeout*/, request_id, &return_value TSRMLS_CC);
	efree(buf->start);

	if (success != 0) {
		/* Exception already thrown */
		return 0;
	}

	return 1;
}

/* {{{ proto MongoWriteBatch MongoWriteBatch::__construct(MongoCollection $collection, long $batch_type)
   Constructs a new Write Batch of $batch_type operations */
PHP_METHOD(MongoWriteBatch, __construct)
{
	long  batch_type;
	zend_error_handling error_handling;
	mongo_write_batch_object *intern;
	zval *zcollection;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ol", &zcollection, mongo_ce_Collection, &batch_type) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	switch(batch_type) {
		case MONGODB_API_COMMAND_INSERT:
		case MONGODB_API_COMMAND_UPDATE:
		case MONGODB_API_COMMAND_DELETE:
			intern->batch_type = batch_type;
			break;
		default:
			zend_throw_exception(mongo_ce_Exception, "Invalid argument, must one of the write methods", 1 TSRMLS_CC);
			return;
	}

	php_mongo_write_batch_ctor(intern, zcollection, batch_type TSRMLS_CC);
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
	php_mongodb_write_item        item;
	php_mongodb_write_update_args update_args;
	php_mongodb_write_delete_args delete_args;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &z_item) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}

	collection = (mongo_collection *)zend_object_store_get_object(intern->zcollection_object TSRMLS_CC);
	connection = get_server(collection, MONGO_CON_FLAG_WRITE TSRMLS_CC);
	zend_restore_error_handling(&error_handling TSRMLS_CC);

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

	if (!php_mongo_api_write_add(&intern->buf, intern->item_count++, &item, connection->max_bson_size TSRMLS_CC)) {
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array MongoWriteBatch::execute(array $writeConcern)
   Executes the constructed batch. Returns the server response */
PHP_METHOD(MongoWriteBatch, execute)
{
	HashTable *write_concern;
	zend_error_handling error_handling;
	mongo_write_batch_object *intern;
	int retval;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "h", &write_concern) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}

	zend_restore_error_handling(&error_handling TSRMLS_CC);

	retval = php_mongo_batch_finalize(&intern->buf, intern->container_pos, intern->batch_pos, intern->zcollection_object, write_concern TSRMLS_CC);
	if (retval == 0) {
		RETURN_FALSE;
	}

	retval = php_mongo_batch_send_and_read(&intern->buf, intern->request_id, intern->zcollection_object, return_value TSRMLS_CC);

	/* Reset the request_id, we use it to know if we need to free stuff during dtor */
	intern->request_id = 0;

	if (retval == 0) {
		RETURN_FALSE;
	}
}
/* }}} */

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(0, collection, MongoCollection, 0)
	ZEND_ARG_INFO(0, batch_type)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_add, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, item, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_execute, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, writeConcern, 0)
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
