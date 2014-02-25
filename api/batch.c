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
#include <zend_exceptions.h>

#include "../php_mongo.h"
#include "../bson.h"
#include "../collection.h" /* mongo_apply_implicit_write_options() */
#include "batch.h"

/* Exceptions thrown */
extern zend_class_entry *mongo_ce_Exception;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

/* Allocates and bootstraps a new php_mongo_batch and its mongo_buffer.
 * The buffer is ready for adding documents as the wire header and command start has been written */
void php_mongo_api_batch_make(mongo_write_batch_object *intern, char *dbname, char *collectionname, php_mongo_write_types type TSRMLS_DC) /* {{{ */
{
	php_mongo_batch *batch = ecalloc(1, sizeof(php_mongo_batch));
	char *cmd_ns;

	CREATE_BUF(batch->buffer, INITIAL_BUF_SIZE);
	batch->request_id = MonGlo(request_id);

	spprintf(&cmd_ns, 0, "%s.$cmd", dbname);
	batch->container_pos = php_mongo_api_write_header(&batch->buffer, cmd_ns TSRMLS_CC);
	batch->batch_pos     = php_mongo_api_write_start(&batch->buffer, type, collectionname TSRMLS_CC);
	efree(cmd_ns);

	if (intern->batch) {
		batch->first = intern->batch->first;
		intern->batch->next = batch;
		intern->batch = batch;
	} else {
		intern->batch = batch;
		batch->first = intern->batch;
	}
}
/* }}} */

/* Wrapper for php_mongo_api_batch_make(), taking MongoCollection zval to extract
 * the database and collection names */
void php_mongo_api_batch_make_from_collection_object(mongo_write_batch_object *intern, zval *zcollection, php_mongo_write_types type TSRMLS_DC) /* {{{ */
{
	mongo_db *db;
	mongo_collection *collection;

	collection = (mongo_collection *)zend_object_store_get_object(zcollection TSRMLS_CC);
	db         = (mongo_db *)zend_object_store_get_object(collection->parent TSRMLS_CC);

	php_mongo_api_batch_make(intern, Z_STRVAL_P(db->name), Z_STRVAL_P(collection->name), type TSRMLS_CC);
}
/* }}} */

/* Recursively free()s php_mongo_batch and leftover buffers.
 * Should only be called on batch failure, the *current* buffer will not be efree()d
 * (it is already free()d on write failure) */
void php_mongo_api_batch_free(php_mongo_batch *batch) /* {{{ */
{
	while(1) {
		php_mongo_batch *prev;

		prev = batch;
		batch = batch->next;
		efree(prev);

		if (!batch) {
			break;
		}
		efree(batch->buffer.start);
	}
}
/* }}} */

/*
 * Initializes a new mongo_write_batch_object with write options, inherited through
 * MongoCollection and default server options - overwritable by the write_options argument */
void php_mongo_api_batch_ctor(mongo_write_batch_object *intern, zval *zcollection, php_mongo_write_types type, HashTable *write_options TSRMLS_DC) /* {{{ */
{
	mongoclient      *link;
	mongo_collection *collection;

	intern->batch_type = type;
	intern->zcollection_object = zcollection;
	Z_ADDREF_P(zcollection);

	collection = (mongo_collection *)zend_object_store_get_object(zcollection TSRMLS_CC);
	link       = (mongoclient *)zend_object_store_get_object(collection->link TSRMLS_CC);

	mongo_apply_implicit_write_options(&intern->write_options, &link->servers->options, zcollection TSRMLS_CC);
	php_mongo_api_write_options_from_ht(&intern->write_options, write_options TSRMLS_CC);
}

/* }}} */

/*
 * Puts the final touches on a mongo_buffer batch by ending the bulk array and write the
 * write options into the buffer
 *
 * Returns:
 * 0: Failed to wrap up the buffer
 * >0 The full message length */
int php_mongo_api_batch_finalize(mongo_buffer *buf, int container_pos, int batch_pos, int max_bson_size, php_mongo_write_options *write_options TSRMLS_DC) /* {{{ */
{
	int message_length;
	message_length = php_mongo_api_write_end(buf, container_pos, batch_pos, MAX_BSON_WIRE_OBJECT_SIZE(max_bson_size), write_options TSRMLS_CC);

	/* The document is greater then allowed limits, exception already thrown */
	if (message_length == 0) {
		efree(buf->start);
	}
	return message_length;
}
/* }}} */

/*
 * Sends the mongo_buffer over the wire and writes the server return value of request_id into return_value
 *
 * Returns:
 * 0 on failure
 * 1 on success */
int php_mongo_api_batch_send_and_read(mongo_buffer *buf, int request_id, mongo_connection *connection, mongo_server_options *server_options, zval *return_value TSRMLS_DC) /* {{{ */
{
	int               bytes_written;
	char             *error_message;
	int               success;

	if (!connection) {
		return 0;
	}

	bytes_written = MonGlo(manager)->send(connection, server_options, buf->start, buf->pos - buf->start, &error_message);
	if (bytes_written < 1) {
		/* Didn't write anything, something bad must have happened */
		efree(buf->start);
		return 0;
	}

	success = php_mongo_api_get_reply(MonGlo(manager), connection, server_options, 0 /*socket_read_timeout*/, request_id, &return_value TSRMLS_CC);
	efree(buf->start);

	if (success != 0) {
		/* Exception already thrown */
		return 0;
	}

	return 1;
}
/* }}} */

/*
 * Executes a php_mongo_batch
 *
 * Returns:
 * 0 On success
 * 1 On failure when we should not attempt to recover from
 * 2 On failure, but it is ok to send more batches */
int php_mongo_api_batch_execute(php_mongo_batch *batch, php_mongo_write_options *write_options, mongo_connection *connection, mongo_server_options *server_options, zval *return_value TSRMLS_DC) /* {{{ */
{
	int retval;
	zval *batch_retval;

	retval = php_mongo_api_batch_finalize(&batch->buffer, batch->container_pos, batch->batch_pos, connection->max_bson_size, write_options TSRMLS_CC);
	if (retval == 0) {
		return 2;
	}

	MAKE_STD_ZVAL(batch_retval);
	retval = php_mongo_api_batch_send_and_read(&batch->buffer, batch->request_id, connection, server_options, batch_retval TSRMLS_CC);

	if (retval == 0) {
		zval_ptr_dtor(&batch_retval);

		/* When executing an ordered batch we need to bail out as soon as we hit an error */
		if (write_options->ordered) {
			return 1;
		}
	} else {
		/* Only add the results on success.. */
		add_next_index_zval(return_value, batch_retval);
	}

	return 0;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
