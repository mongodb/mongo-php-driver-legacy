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
#include "../api/batch.h"
#include "../batch/write.h"
#include "../batch/write_private.h"

ZEND_EXTERN_MODULE_GLOBALS(mongo)

zend_class_entry *mongo_ce_DeleteBatch = NULL;

/* php_mongo.c */
extern zend_object_handlers mongo_type_object_handlers;
/* collection.c */
extern zend_class_entry *mongo_ce_Collection;
/* See batch/write.c */
extern zend_class_entry *mongo_ce_WriteBatch;

/* {{{ proto MongoDeleteBatch MongoDeleteBatch::__construct(MongoCollection $collection [, array $write_options])
   Constructs a new Write Batch of $batch_type operations */
PHP_METHOD(MongoDeleteBatch, __construct)
{
	zend_error_handling error_handling;
	mongo_write_batch_object *intern;
	HashTable *write_options = NULL;
	zval *zcollection;

	zend_replace_error_handling(EH_THROW, NULL, &error_handling TSRMLS_CC);
	intern = (mongo_write_batch_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|h", &zcollection, mongo_ce_Collection, &write_options) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	php_mongo_api_batch_ctor(intern, zcollection, MONGODB_API_COMMAND_DELETE, write_options TSRMLS_CC);
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(0, collection, MongoCollection, 0)
	ZEND_ARG_ARRAY_INFO(0, write_options, 0)
ZEND_END_ARG_INFO()

static zend_function_entry MongoDeleteBatch_methods[] = {
	PHP_ME(MongoDeleteBatch, __construct, arginfo___construct, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void mongo_init_MongoDeleteBatch(TSRMLS_D)
{
	zend_class_entry delete_batch;
	INIT_CLASS_ENTRY(delete_batch, "MongoDeleteBatch", MongoDeleteBatch_methods);

	delete_batch.create_object = php_mongo_write_batch_object_new;

	mongo_ce_DeleteBatch = zend_register_internal_class_ex(&delete_batch, mongo_ce_WriteBatch, "MongoWriteBatch" TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
