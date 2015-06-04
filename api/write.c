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
#include "../cursor_shared.h" /* For php_mongo_cursor_throw() */
#include "../log_stream.h"
#include "write.h"

#undef fsync

/* Exceptions thrown */
extern zend_class_entry *mongo_ce_Exception;
extern zend_class_entry *mongo_ce_CursorException;
extern zend_class_entry *mongo_ce_CursorTimeoutException;
extern zend_class_entry *mongo_ce_ProtocolException;
extern zend_class_entry *mongo_ce_WriteConcernException;
extern zend_class_entry *mongo_ce_DuplicateKeyException;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

static void php_mongo_api_add_write_options(mongo_buffer *buf, php_mongo_write_options *write_options TSRMLS_DC);
static void php_mongo_api_throw_exception(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC);
static void php_mongo_api_throw_exception_from_server_code(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC);
static int php_mongo_api_insert_add(mongo_buffer *buf, int n, HashTable *document, int max_document_size TSRMLS_DC);
static int php_mongo_api_update_add(mongo_buffer *buf, int n, php_mongo_write_update_args *update_args, int max_document_size TSRMLS_DC);
static int php_mongo_api_delete_add(mongo_buffer *buf, int n, php_mongo_write_delete_args *delete_args, int max_document_size TSRMLS_DC);

/* Wrapper for php_mongo_api_write_options_from_ht(), taking a zval rather then HashTable */
void php_mongo_api_write_options_from_zval(php_mongo_write_options *write_options, zval *z_write_options TSRMLS_DC) /* {{{ */
{

	if (!z_write_options) {
		return;
	}

	php_mongo_api_write_options_from_ht(write_options, HASH_P(z_write_options) TSRMLS_CC);
}
/* }}} */

/* Converts a HashTable of $options (passed to ->insert(), ->update(), ...) and creates a
 * php_mongo_write_options() from it */
void php_mongo_api_write_options_from_ht(php_mongo_write_options *write_options, HashTable *hindex TSRMLS_DC) /* {{{ */
{
	HashPosition pointer;
	zval **data;
	char *key;
	uint index_key_len;
	ulong index;

	if (!hindex) {
		return;
	}

	for (
			zend_hash_internal_pointer_reset_ex(hindex, &pointer);
			zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(hindex, &pointer)
		) {
		uint key_type = zend_hash_get_current_key_ex(hindex, &key, &index_key_len, &index, NO_DUP, &pointer);

		if (key_type == HASH_KEY_IS_LONG) {
			continue;
		}

		if (zend_binary_strcasecmp(key, index_key_len, "ordered", strlen("ordered") + 1) == 0) {
			write_options->ordered = zend_is_true(*data);
		}
		else if (zend_binary_strcasecmp(key, index_key_len, "fsync", strlen("fsync") + 1) == 0) {
			write_options->fsync = zend_is_true(*data);
		}
		else if (zend_binary_strcasecmp(key, index_key_len, "j", strlen("j") + 1) == 0) {
			write_options->j = zend_is_true(*data);
		}
		else if (zend_binary_strcasecmp(key, index_key_len, "wTimeoutMS", strlen("wTimeoutMS") + 1) == 0) {
			convert_to_long_ex(data);
			write_options->wtimeout = Z_LVAL_PP(data);
		}
		else if (zend_binary_strcasecmp(key, index_key_len, "wtimeout", strlen("wtimeout") + 1) == 0) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'wtimeout' option is deprecated, please use 'wTimeoutMS' instead");
			convert_to_long_ex(data);
			write_options->wtimeout = Z_LVAL_PP(data);
		}
		else if (zend_binary_strcasecmp(key, index_key_len, "safe", strlen("safe") + 1) == 0) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'safe' option is deprecated, please use 'w' instead");
			if (Z_TYPE_PP(data) == IS_LONG || Z_TYPE_PP(data) == IS_BOOL) {
				if (write_options->wtype == 1 && write_options->write_concern.w > Z_LVAL_PP(data)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Using w=%d rather than w=%ld as suggested by deprecated 'safe' value", write_options->write_concern.w, Z_LVAL_PP(data));
				} else {
					write_options->write_concern.w = Z_LVAL_PP(data);
					write_options->wtype = 1;
				}
			} else {
				convert_to_string_ex(data);
				write_options->write_concern.wstring = Z_STRVAL_PP(data);
				write_options->wtype = 2;
			}
		}
		else if (zend_binary_strcasecmp(key, index_key_len, "w", strlen("w") + 1) == 0) {
			if (Z_TYPE_PP(data) == IS_LONG || Z_TYPE_PP(data) == IS_BOOL) {
				write_options->write_concern.w = Z_LVAL_PP(data);
				write_options->wtype = 1;
			} else {
				convert_to_string_ex(data);
				write_options->write_concern.wstring = Z_STRVAL_PP(data);
				write_options->wtype = 2;
			}
		}
	}
}
/* }}} */

/* Converts a php_mongo_write_options to a zval representation of the structure */
void php_mongo_api_write_options_to_zval(php_mongo_write_options *write_options, zval *z_write_options) /* {{{ */
{
	zval *write_concern;

	if (write_options->ordered != -1) {
		add_assoc_bool(z_write_options, "ordered", write_options->ordered);
	}

	MAKE_STD_ZVAL(write_concern);
	array_init(write_concern);

	if (write_options->fsync != -1) {
		add_assoc_bool(write_concern, "fsync", write_options->fsync);
	}

	if (write_options->j != -1) {
		add_assoc_bool(write_concern, "j", write_options->j);
	}

	if (write_options->wtimeout != -1) {
		add_assoc_long(write_concern, "wtimeout", write_options->wtimeout);
	}

	if (write_options->wtype == 1) {
		add_assoc_long(write_concern, "w", write_options->write_concern.w);
	} else if (write_options->wtype == 2) {
		add_assoc_string(write_concern, "w", write_options->write_concern.wstring, 1);
	}

	add_assoc_zval(z_write_options, "writeConcern", write_concern);

}
/* }}} */

/* Internal helper: Writes the CRUD key from `type` to `buf` */
void php_mongo_api_write_command_name(mongo_buffer *buf, php_mongo_write_types type TSRMLS_DC) /* {{{ */
{
	switch (type) {
		case MONGODB_API_COMMAND_INSERT:
			php_mongo_serialize_key(buf, "insert", strlen("insert"), 0 TSRMLS_CC);
			break;
		case MONGODB_API_COMMAND_UPDATE:
			php_mongo_serialize_key(buf, "update", strlen("update"), 0 TSRMLS_CC);
			break;
		case MONGODB_API_COMMAND_DELETE:
			php_mongo_serialize_key(buf, "delete", strlen("delete"), 0 TSRMLS_CC);
			break;
	}
}
/* }}} */

/* Internal helper: Writes the batch keyname for that `type` to `buf` */
void php_mongo_api_write_command_fieldname(mongo_buffer *buf, php_mongo_write_types type TSRMLS_DC) /* {{{ */
{
	switch (type) {
		case MONGODB_API_COMMAND_INSERT:
			php_mongo_serialize_key(buf, "documents", strlen("documents"), 0 TSRMLS_CC);
			break;
		case MONGODB_API_COMMAND_UPDATE:
			php_mongo_serialize_key(buf, "updates", strlen("updates"), 0 TSRMLS_CC);
			break;
		case MONGODB_API_COMMAND_DELETE:
			php_mongo_serialize_key(buf, "deletes", strlen("deletes"), 0 TSRMLS_CC);
			break;
	}
}
/* }}} */

/* 
 * Returns the position of the root element, needed to backtrack and serialize
 * the size of the combined command */
int php_mongo_api_write_header(mongo_buffer *buf, char *ns TSRMLS_DC) /* {{{ */
{
	int container_pos;

	/* Standard Message Header */
	buf->pos += INT_32;                                 /* skip messageLength */
	php_mongo_serialize_int(buf, MonGlo(request_id)++); /* requestID */
	php_mongo_serialize_int(buf, 0);                    /* responseTo */
	php_mongo_serialize_int(buf, OP_QUERY);             /* opCode */

	/* Start OP_QUERY */
	php_mongo_serialize_int(buf, 0);                    /* flags */
	php_mongo_serialize_ns(buf, ns TSRMLS_CC);          /* namespace */
	php_mongo_serialize_int(buf, 0);                    /* numberToSkip */
	php_mongo_serialize_int(buf, -1);                   /* numberToReturn */

	/* The root element is a BSON Document, which doesn't have a type
	 * But we need to reserve space to fill out its size.
	 * We need to store our current position so we can write it in the _end() */
	container_pos = buf->pos-buf->start;
	buf->pos += INT_32;

	return container_pos;
}
/* }}} */

/* Bootstraps a Write API message with the correct command format + fieldnames
 * Expects php_mongo_api_write_header() has been called successfully first.
 * Returns the position of the items object where the size needs to be filled in later */
int php_mongo_api_write_start(mongo_buffer *buf, php_mongo_write_types type, char *collection TSRMLS_DC) /* {{{ */
{
	int object_pos;

	/* <command-name>: databasename.collectionName */
	php_mongo_set_type(buf, BSON_STRING);
	php_mongo_api_write_command_name(buf, type TSRMLS_CC);
	php_mongo_serialize_int(buf, strlen(collection) + 1);
	php_mongo_serialize_string(buf, collection, strlen(collection));

	/* documents: [ 0: {document: 1},                        1: {document: 2},                         ...] */
	/* updates:   [ 0: { q: {}, u: {}, multi: b, upsert: b}, 1: { q: {}, u: {}, multi: b, upsert: b }, ...] */
	/* deletes:   [ 0: { q: {},        limit: n},            1: { q: {},        limit: n            }, ...] */
	php_mongo_set_type(buf, BSON_ARRAY);
	php_mongo_api_write_command_fieldname(buf, type TSRMLS_CC);

	object_pos = buf->pos-buf->start;
	buf->pos += INT_32;
	return object_pos;
}
/* }}}  */

/* Adds a new document to write. Must be called after php_mongo_api_write_start(), and
 * can be called many many times.
 * n: The index of the document (eg: n=0 for first document, n=1 for second doc in a batch)
 * document: The actual document to insert
 * max_document_size: The max document size allowed
 *
 * Will add _id to the document if not present.
 * Throws mongo_ce_Exception on failure (not UTF-8, \0 in key.., overflow max_document_size)
 *
 *
 * If the document overflows the allowed wire transfer size, the buffer position will be
 * reset to its original location and we pretend nothing was ever done
 *
 * Returns:
 * SUCCESS on success
 * FAILURE on failure serialization validation failure
 * 2 when document would overflow wire transfer size, and will not be added
 */
int php_mongo_api_write_add(mongo_buffer *buf, int n, php_mongo_write_item *item, int max_document_size TSRMLS_DC) /* {{{  */
{
	int retval;
	int rollbackpos = buf->pos - buf->start;

	switch (item->type) {
		case MONGODB_API_COMMAND_INSERT:
			retval = php_mongo_api_insert_add(buf, n, item->write.insert_doc, max_document_size TSRMLS_CC);
			break;

		case MONGODB_API_COMMAND_UPDATE:
			retval = php_mongo_api_update_add(buf, n, item->write.update_args, max_document_size TSRMLS_CC);
			break;

		case MONGODB_API_COMMAND_DELETE:
			retval = php_mongo_api_delete_add(buf, n, item->write.delete_args, max_document_size TSRMLS_CC);
			break;
	}

	/* Writing the object to the buffer failed and raised an exception, bail out */
	if (retval == 0) {
		return FAILURE;
	}

	/* Rollback the buffer, effectively removing this document from it */
	if (buf->pos - buf->start > MAX_BSON_WIRE_OBJECT_SIZE(max_document_size)) {
		buf->pos = buf->start + rollbackpos;
		return 2;
	}

	return SUCCESS;
}
/* }}} */

/* Internal helper for the php_mongo_api_write_add() helper */
static int php_mongo_api_insert_add(mongo_buffer *buf, int n, HashTable *document, int max_document_size TSRMLS_DC) /* {{{  */
{
	char *number;
	php_mongo_set_type(buf, BSON_OBJECT);

	spprintf(&number, 0, "%d", n);
	php_mongo_serialize_key(buf, number, strlen(number), 0 TSRMLS_CC);
	efree(number);

	/* PREP=add _id if it doesn't exist */
	if (zval_to_bson(buf, document, PREP, max_document_size TSRMLS_CC) == FAILURE) {
		return 0;
	}

	return 1;
}
/* }}}  */

/* Internal helper for the php_mongo_api_write_add() helper */
static int php_mongo_api_update_add(mongo_buffer *buf, int n, php_mongo_write_update_args *update_args, int max_document_size TSRMLS_DC) /* {{{  */
{
	int argstart, document_end;
	char *number;

	php_mongo_set_type(buf, BSON_OBJECT);

	spprintf(&number, 0, "%d", n);
	php_mongo_serialize_key(buf, number, strlen(number), 0 TSRMLS_CC);
	efree(number);

	argstart = buf->pos-buf->start;
	buf->pos += INT_32;

	php_mongo_set_type(buf, BSON_OBJECT);
	php_mongo_serialize_key(buf, "q", strlen("q"), 0 TSRMLS_CC);
	if (zval_to_bson(buf, HASH_OF(update_args->query), NO_PREP, max_document_size TSRMLS_CC) == FAILURE) {
		return 0;
	}
	php_mongo_set_type(buf, BSON_OBJECT);
	php_mongo_serialize_key(buf, "u", strlen("u"), 0 TSRMLS_CC);
	if (zval_to_bson(buf, HASH_OF(update_args->update), NO_PREP, max_document_size TSRMLS_CC) == FAILURE) {
		return 0;
	}

	if (update_args->multi != -1) {
		php_mongo_set_type(buf, BSON_BOOL);
		php_mongo_serialize_key(buf, "multi", strlen("multi"), 0 TSRMLS_CC);
		php_mongo_serialize_bool(buf, update_args->multi);
	}

	if (update_args->upsert != -1) {
		php_mongo_set_type(buf, BSON_BOOL);
		php_mongo_serialize_key(buf, "upsert", strlen("upsert"), 0 TSRMLS_CC);
		php_mongo_serialize_bool(buf, update_args->upsert);
	}

	/* Finished with the object */
	php_mongo_serialize_null(buf);

	/* Seek back and set the size of this object */
	document_end = MONGO_32((buf->pos - (buf->start + argstart)));
	memcpy(buf->start + argstart, &document_end, INT_32);

	return 1;
}
/* }}}  */

/* Internal helper for the php_mongo_api_write_add() helper */
static int php_mongo_api_delete_add(mongo_buffer *buf, int n, php_mongo_write_delete_args *delete_args, int max_document_size TSRMLS_DC) /* {{{  */
{
	int argstart, document_end;
	char *number;

	php_mongo_set_type(buf, BSON_OBJECT);

	spprintf(&number, 0, "%d", n);
	php_mongo_serialize_key(buf, number, strlen(number), 0 TSRMLS_CC);
	efree(number);

	argstart = buf->pos-buf->start;
	buf->pos += INT_32;

	php_mongo_set_type(buf, BSON_OBJECT);
	php_mongo_serialize_key(buf, "q", strlen("q"), 0 TSRMLS_CC);
	if (zval_to_bson(buf, HASH_OF(delete_args->query), NO_PREP, max_document_size TSRMLS_CC) == FAILURE) {
		return 0;
	}

	if (delete_args->limit != -1) {
		php_mongo_set_type(buf, BSON_INT);
		php_mongo_serialize_key(buf, "limit", strlen("limit"), 0 TSRMLS_CC);
		php_mongo_serialize_int(buf, delete_args->limit);
	}

	/* Finished with the object */
	php_mongo_serialize_null(buf);

	/* Seek back and set the size of this object */
	document_end = MONGO_32((buf->pos - (buf->start + argstart)));
	memcpy(buf->start + argstart, &document_end, INT_32);

	return 1;
}
/* }}}  */

/* Finalize the BSON buffer.
 * Requires the container_pos from php_mongo_api_write_start() and the max_write_size
 * Use MAX_BSON_WIRE_OBJECT_SIZE(max_bson_size) to calculate the correct max_write_size
 * Returns the the full message length.
 * Throws mongo_ce_Exception if the buffer is larger then max_write_size */
int php_mongo_api_write_end(mongo_buffer *buf, int container_pos, int batch_pos, int max_write_size, php_mongo_write_options *write_options TSRMLS_DC) /* {{{  */
{

	php_mongo_serialize_null(buf);

	if (php_mongo_serialize_size(buf->start+batch_pos, buf, max_write_size/*MAX_BSON_WIRE_OBJECT_SIZE(max_document_size)*/ TSRMLS_CC) == FAILURE) {
		return 0;
	}

	if (write_options) {
		php_mongo_api_add_write_options(buf, write_options TSRMLS_CC);
	}

	php_mongo_serialize_null(buf);
	if (php_mongo_serialize_size(buf->start + container_pos, buf, max_write_size TSRMLS_CC) == FAILURE) {
		return 0;
	}

	if (php_mongo_serialize_size(buf->start, buf, max_write_size TSRMLS_CC)) {
		return 0;
	}

	return MONGO_32((buf->pos - buf->start));

}
/* }}}  */

/* Helper to insert a single document.
 * Does the _start() _add() _end() dance.
 * Returns the generated Protocol Request ID (>0) on success
 * Returns 0 on failure, and raises exception (see _add() and _end()) */
int php_mongo_api_insert_single(mongo_buffer *buf, char *ns, char *collection, zval *document, php_mongo_write_options *write_options, mongo_connection *connection TSRMLS_DC) /* {{{  */
{
	int request_id;
	int container_pos, batch_pos;
	int message_length;

	request_id = MonGlo(request_id);
	container_pos = php_mongo_api_write_header(buf, ns TSRMLS_CC);
	batch_pos = php_mongo_api_write_start(buf, MONGODB_API_COMMAND_INSERT, collection TSRMLS_CC);

	if (!php_mongo_api_insert_add(buf, 0, HASH_OF(document), connection->max_bson_size TSRMLS_CC)) {
		return 0;
	}

	message_length = php_mongo_api_write_end(buf, container_pos, batch_pos, MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size), write_options TSRMLS_CC);
	/* Overflowed the max_write_size */
	if (message_length == 0) {
		return 0;
	}

	mongo_log_stream_cmd_insert(connection, document, write_options, message_length, request_id, ns TSRMLS_CC);

	return request_id;
}
/* }}}  */

/* Helper to insert a single document.
 * Does the _start() _add() _end() dance.
 * Returns the generated Protocol Request ID (>0) on success
 * Returns 0 on failure, and raises exception (see _add() and _end()) */
int php_mongo_api_delete_single(mongo_buffer *buf, char *ns, char *collection, php_mongo_write_delete_args *delete_args, php_mongo_write_options *write_options, mongo_connection *connection TSRMLS_DC) /* {{{ */
{
	int request_id;
	int container_pos, batch_pos;
	int message_length;

	request_id = MonGlo(request_id);
	container_pos = php_mongo_api_write_header(buf, ns TSRMLS_CC);
	batch_pos = php_mongo_api_write_start(buf, MONGODB_API_COMMAND_DELETE, collection TSRMLS_CC);

	if (!php_mongo_api_delete_add(buf, 0, delete_args, connection->max_bson_size TSRMLS_CC)) {
		return 0;
	}

	message_length = php_mongo_api_write_end(buf, container_pos, batch_pos, MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size), write_options TSRMLS_CC);
	/* Overflowed the max_write_size */
	if (message_length == 0) {
		return 0;
	}

	mongo_log_stream_cmd_delete(connection, delete_args, write_options, message_length, request_id, ns TSRMLS_CC);

	return request_id;
}
/* }}} */

/* Helper to update a single document.
 * Does the _start() _add() _end() dance.
 * Returns the generated Protocol Request ID (>0) on success
 * Returns 0 on failure, and raises exception (see _add() and _end()) */
int php_mongo_api_update_single(mongo_buffer *buf, char *ns, char *collection, php_mongo_write_update_args *update_args, php_mongo_write_options *write_options, mongo_connection *connection TSRMLS_DC) /* {{{ */
{
	int request_id;
	int container_pos, batch_pos;
	int message_length;

	request_id = MonGlo(request_id);
	container_pos = php_mongo_api_write_header(buf, ns TSRMLS_CC);
	batch_pos = php_mongo_api_write_start(buf, MONGODB_API_COMMAND_UPDATE, collection TSRMLS_CC);

	if (!php_mongo_api_update_add(buf, 0, update_args, connection->max_bson_size TSRMLS_CC)) {
		return 0;
	}

	message_length = php_mongo_api_write_end(buf, container_pos, batch_pos, MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size), write_options TSRMLS_CC);
	/* Overflowed the max_write_size */
	if (message_length == 0) {
		return 0;
	}

	mongo_log_stream_cmd_update(connection, update_args, write_options, message_length, request_id, ns TSRMLS_CC);

	return request_id;
}
/* }}} */

/* Returns 0 on success
 * Returns 1 on failure, and sets EG(exception)
 * Returns 2 on failure, and sets EG(exception), leaves retval dangling */
int php_mongo_api_get_reply(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *options, int socket_read_timeout, int request_id, zval **retval TSRMLS_DC) /* {{{  */
{
	int                status = 0;
	char               buf[REPLY_HEADER_SIZE];
	char              *data;
	size_t             data_len = 0;
	char              *error_message;
	mongo_msg_header   msg_header;
	php_mongo_reply  dbreply;

	status = manager->recv_header(connection, options, socket_read_timeout, buf, REPLY_HEADER_LEN, &error_message);
	if (status < 0) {
		/* Read failed, error message populated by recv_header */
		php_mongo_api_throw_exception(connection, abs(status), error_message, NULL TSRMLS_CC);
		free(error_message);
		return 1;
	} else if (status < INT_32*4) {
		spprintf(&error_message, 256, "couldn't get full response header, got %d bytes but expected atleast %d", status, INT_32*4);
		php_mongo_api_throw_exception(connection, 4, error_message, NULL TSRMLS_CC);
		free(error_message);
		return 1;
	}

	msg_header.length      = MONGO_32(*(int*)buf);
	msg_header.request_id  = MONGO_32(*(int*)(buf + INT_32));
	msg_header.response_to = MONGO_32(*(int*)(buf + INT_32*2));
	msg_header.op          = MONGO_32(*(int*)(buf + INT_32*3));
	dbreply.flags          = MONGO_32(*(int*)(buf + INT_32*4));
	dbreply.cursor_id      = MONGO_64(*(int64_t*)(buf + INT_32*5));
	dbreply.start          = MONGO_32(*(int*)(buf + INT_32*5 + INT_64));
	dbreply.returned       = MONGO_32(*(int*)(buf + INT_32*6 + INT_64));

	mongo_log_stream_write_reply(connection, &msg_header, &dbreply TSRMLS_CC);

	if (msg_header.length < REPLY_HEADER_SIZE) {
		spprintf(&error_message, 256, "bad response length: %d, did the db assert?", msg_header.length);
		php_mongo_api_throw_exception(connection, 6, error_message, NULL TSRMLS_CC);
		free(error_message);
		return 1;
	}
	if (msg_header.length > MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size)) {
		spprintf(&error_message, 0, "Message size (%d) overflows valid message size (%d)", msg_header.length, MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size));
		php_mongo_api_throw_exception(connection, 35, error_message, NULL TSRMLS_CC);
		free(error_message);
		return 1;
	}
	if (msg_header.response_to != request_id) {
		spprintf(&error_message, 0, "request/response mismatch: %d vs %d", request_id, msg_header.response_to);
		php_mongo_api_throw_exception(connection, 36, error_message, NULL TSRMLS_CC);
		free(error_message);
		return 1;
	}

	data_len = msg_header.length - REPLY_HEADER_LEN;
	data = (char*)emalloc(data_len);

	status = manager->recv_data(connection, options, 0, data, data_len, &error_message);
	if (status < 0) {
		php_mongo_api_throw_exception(connection, 37, error_message, NULL TSRMLS_CC);
		free(error_message);
		return 1;
	}

	bson_to_zval_iter(data, data_len, Z_ARRVAL_PP(retval), 0 TSRMLS_CC);
	efree(data);

	return 0;
}
/* }}} */

/* Internal helper: Writes the php_mongo_write_options options to the buffer */
static void php_mongo_api_add_write_options(mongo_buffer *buf, php_mongo_write_options *write_options TSRMLS_DC) /* {{{  */
{
	int document_start, document_end;

	if (write_options->ordered != -1) {
		php_mongo_set_type(buf, BSON_BOOL);
		php_mongo_serialize_key(buf, "ordered", strlen("ordered"), 0 TSRMLS_CC);
		php_mongo_serialize_bool(buf, write_options->ordered);
	}

	php_mongo_set_type(buf, BSON_OBJECT);
	php_mongo_serialize_key(buf, "writeConcern", strlen("writeConcern"), NO_PREP TSRMLS_CC);

	/* Record the document start, and leave space for setting the size later */
	document_start = buf->pos-buf->start;
	buf->pos += INT_32;

	if (write_options->fsync != -1) {
		php_mongo_set_type(buf, BSON_BOOL);
		php_mongo_serialize_key(buf, "fsync", strlen("fsync"), 0 TSRMLS_CC);
		php_mongo_serialize_bool(buf, write_options->fsync);
	}

	if (write_options->j != -1) {
		php_mongo_set_type(buf, BSON_BOOL);
		php_mongo_serialize_key(buf, "j", strlen("j"), 0 TSRMLS_CC);
		php_mongo_serialize_bool(buf, write_options->j);
	}

	if (write_options->wtimeout != -1) {
		php_mongo_set_type(buf, BSON_INT);
		php_mongo_serialize_key(buf, "wtimeout", strlen("wtimeout"), 0 TSRMLS_CC);
		php_mongo_serialize_int(buf, write_options->wtimeout);
	}

	if (write_options->wtype == 1) {
		php_mongo_set_type(buf, BSON_INT);
		php_mongo_serialize_key(buf, "w", strlen("w"), 0 TSRMLS_CC);
		php_mongo_serialize_int(buf, write_options->write_concern.w);
	} else if (write_options->wtype == 2) {
		php_mongo_set_type(buf, BSON_STRING);
		php_mongo_serialize_key(buf, "w", strlen("w"), 0 TSRMLS_CC);
		php_mongo_serialize_int(buf, strlen(write_options->write_concern.wstring) + 1);
		php_mongo_serialize_string(buf, write_options->write_concern.wstring, strlen(write_options->write_concern.wstring));
	}

	/* Finished with the object */
	php_mongo_serialize_null(buf);

	/* Seek back and set the size of this object */
	document_end = MONGO_32((buf->pos - (buf->start + document_start)));
	memcpy(buf->start + document_start, &document_end, INT_32);
}
/* }}}  */

/* Internal helper: Called from php_mongo_api_raise_exception_on_write_failure() to exception on complete command failure */
static void php_mongo_api_raise_epic_write_failure_exception(mongo_connection *connection, zval *document TSRMLS_DC) /* {{{ */
{
	zval **code, **errmsg;

	if (zend_hash_find(Z_ARRVAL_P(document), "code", strlen("code") + 1, (void**)&code) == SUCCESS) {
		convert_to_long(*code);

		if (zend_hash_find(Z_ARRVAL_P(document), "errmsg", strlen("errmsg") + 1, (void**)&errmsg) == SUCCESS) {
			convert_to_string(*errmsg);

			php_mongo_api_throw_exception_from_server_code(connection, Z_LVAL_PP(code), Z_STRVAL_PP(errmsg), document TSRMLS_CC);
		} else {
			php_mongo_api_throw_exception_from_server_code(connection, Z_LVAL_PP(code), "Unknown failure, no error message from server", document TSRMLS_CC);
		}
	}
	/* Couldn't find a error code.. do I atleast have a error message? */
	else if (zend_hash_find(Z_ARRVAL_P(document), "errmsg", strlen("errmsg") + 1, (void**)&errmsg) == SUCCESS) {
		convert_to_string(*errmsg);

		php_mongo_api_throw_exception_from_server_code(connection, 100, Z_STRVAL_PP(errmsg), document TSRMLS_CC);
	}
	else {
		php_mongo_api_throw_exception_from_server_code(connection, 101, "Unknown error occurred, did not get an error message or code", document TSRMLS_CC);
	}
}
/* }}} */

/* Internal helper: Called from php_mongo_api_raise_exception_on_write_failure() to raise maybe multiple exceptions */
static void php_mongo_api_raise_exception_it_array(mongo_connection *connection, zval **z_write_errors, zval *document TSRMLS_DC) /* {{{ */
{
	HashTable  *write_errors = Z_ARRVAL_PP(z_write_errors);
	zval      **error;

	zend_hash_internal_pointer_reset(write_errors);
	while (zend_hash_get_current_data(write_errors, (void **)&error) == SUCCESS) {
		zval **index = NULL, **code = NULL, **errmsg = NULL;

		if (Z_TYPE_PP(error) != IS_ARRAY) {
			php_mongo_api_throw_exception_from_server_code(connection, 102, "Got write errors, but don't know how to parse them", *error TSRMLS_CC);
			break;
		}

		if (zend_hash_find(Z_ARRVAL_PP(error), "index", strlen("index") + 1, (void**)&index) == SUCCESS) {
			convert_to_long_ex(index);
		}
		if (zend_hash_find(Z_ARRVAL_PP(error), "code", strlen("code") + 1, (void**)&code) == SUCCESS) {
			convert_to_long_ex(code);
		}
		if (zend_hash_find(Z_ARRVAL_PP(error), "errmsg", strlen("errmsg") + 1, (void**)&errmsg) == SUCCESS) {
			convert_to_string_ex(errmsg);
		}

		/* FIXME: Do we care about the index? */
		php_mongo_api_throw_exception_from_server_code(connection, Z_LVAL_PP(code), Z_STRVAL_PP(errmsg), document TSRMLS_CC);
		zend_hash_move_forward(write_errors);
	}
}
/* }}} */

/* Parses an command response document to see if it contains errors
 * Returns 0 when write succeeded (no write errors)
 * Returns 1 when write failed, and raises different exceptions depending on the error */
int php_mongo_api_raise_exception_on_write_failure(mongo_connection *connection, zval *document TSRMLS_DC) /* {{{ */
{
	zval **write_errors, **write_concern_error;

	/* Check for Write Errors, such as duplicate key errors */
	if (zend_hash_find(Z_ARRVAL_P(document), "writeErrors", strlen("writeErrors") + 1, (void**)&write_errors) == SUCCESS) {
		if (Z_TYPE_PP(write_errors) == IS_ARRAY) {
			php_mongo_api_raise_exception_it_array(connection, write_errors, document TSRMLS_CC);
		} else {
			php_mongo_api_throw_exception_from_server_code(connection, 104, "Unexpected server response: 'writeErrors' is not an array", document TSRMLS_CC);
		}
		return 1;
	}

	/* Write Concern Error, such as wtimeout */
	if (zend_hash_find(Z_ARRVAL_P(document), "writeConcernError", strlen("writeConcernError") + 1, (void**)&write_concern_error) == SUCCESS) {
		zval **code = NULL, **errmsg = NULL;

		if (Z_TYPE_PP(write_concern_error) == IS_ARRAY) {
			if (zend_hash_find(Z_ARRVAL_PP(write_concern_error), "code", strlen("code") + 1, (void**)&code) == SUCCESS) {
				convert_to_long_ex(code);
			}
			if (zend_hash_find(Z_ARRVAL_PP(write_concern_error), "errmsg", strlen("errmsg") + 1, (void**)&errmsg) == SUCCESS) {
				convert_to_string_ex(errmsg);
			}

			/* FIXME: There is also errorInfo.. Do we care? We include the full error document anyway.. */
			php_mongo_api_throw_exception_from_server_code(connection, Z_LVAL_PP(code), Z_STRVAL_PP(errmsg), document TSRMLS_CC);
		} else {
			php_mongo_api_throw_exception_from_server_code(connection, 105, "Unexpected server response: 'writeConcernError' is not an array", document TSRMLS_CC);
		}
		return 1;
	}

	return 0;
}
/* }}} */

/* Parses a command response document and raises an exception on write command failure.
 * Returns 0 when writecommand succeded (ok=1)
 * Returns 1 on failure, and throws exception */
int php_mongo_api_raise_exception_on_command_failure(mongo_connection *connection, zval *document TSRMLS_DC) /* {{{ */
{
	zval **ok;

	if (zend_hash_find(Z_ARRVAL_P(document), "ok", strlen("ok") + 1, (void**)&ok) == SUCCESS) {
		convert_to_boolean(*ok);

		/* If ok=false we should have a top-level code and errmsg fields
		 * These would be epic failures, authentication, parse errors.. */
		if (!Z_BVAL_PP(ok)) {
			php_mongo_api_raise_epic_write_failure_exception(connection, document TSRMLS_CC);
			return 1;
		}
	} else {
		/* Missing OK field... that can't be good! */
		php_mongo_api_throw_exception_from_server_code(connection, 103, "Unexpected server response: 'ok' field is not present", document TSRMLS_CC);
		return 1;
	}

	return 0;
}
/* }}} */

/* Internal helper: raises exception based on the server error code */
static void php_mongo_api_throw_exception_from_server_code(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC) /* {{{ */
{
	zval *exception;

	/* php_mongo_cursor_throw() rewrites the ce for certain error codes */
	exception = php_mongo_cursor_throw(mongo_ce_WriteConcernException, connection, code TSRMLS_CC, "%s", error_message);

	if (document && Z_TYPE_P(document) == IS_ARRAY) {
		zval *error_doc;

		MAKE_STD_ZVAL(error_doc);
		array_init(error_doc);
		zend_hash_copy(Z_ARRVAL_P(error_doc), Z_ARRVAL_P(document), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		zend_update_property(Z_OBJCE_P(exception), exception, "document", strlen("document"), error_doc TSRMLS_CC);
		zval_ptr_dtor(&error_doc);
	}

}
/* }}}  */

/* Internal helper: raises exception based on our failure codes */
static void php_mongo_api_throw_exception(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC) /* {{{ */
{
	zval *exception;
	zend_class_entry *ce;

	switch (code) {
		case 2: /* old-style timeout? I don't know where this is coming from */
		case 80: /* timeout, io_stream:php_mongo_io_stream_read() */
			ce = mongo_ce_CursorTimeoutException; /* Cursor Exception for BC */
			break;

		case 32: /* Remote server has closed the connection, io_stream:php_mongo_io_stream_read() */
			ce = mongo_ce_CursorException; /* Cursor Exception for BC */
			break;

			/* MongoCursorException for BC with pre-2.6 write API*/
		case 4: /* couldn't get full response header, got %d bytes but expected atleast %d php_mongo_api_get_reply() */
		case 6: /* bad response length: %d, did the db assert? php_mongo_api_get_reply() */
		case 35: /* Message size (%d) overflows valid message size (%d) php_mongo_api_get_reply() */
		case 36: /* request/response mismatch: %d vs %d php_mongo_api_get_reply() */
		case 37: /* Couldn't finish reading from network */
			ce = mongo_ce_CursorException; /* should be MongoProtocol error php_mongo_api_get_reply() */
			break;

		default:
			ce = mongo_ce_ProtocolException;
			break;
	}

	exception = php_mongo_cursor_throw(ce, connection, code TSRMLS_CC, "%s", error_message);

	if (document && Z_TYPE_P(document) == IS_ARRAY) {
		zval *error_doc;

		MAKE_STD_ZVAL(error_doc);
		array_init(error_doc);
		zend_hash_copy(Z_ARRVAL_P(error_doc), Z_ARRVAL_P(document), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		zend_update_property(ce, exception, "document", strlen("document"), error_doc TSRMLS_CC);
		zval_ptr_dtor(&error_doc);
	}

}
/* }}}  */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
