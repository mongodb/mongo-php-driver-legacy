/**
 *  Copyright 2013 MongoDB, Inc.
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

#include <zend_exceptions.h>
#include "../php_mongo.h"
#include "../bson.h"
#include "../cursor.h" /* For mongo_cursor_throw() */
#include "../log_stream.h"
#include "write.h"

/* Exceptions thrown */
extern zend_class_entry *mongo_ce_Exception;
extern zend_class_entry *mongo_ce_CursorException;
extern zend_class_entry *mongo_ce_CursorTimeoutException;
extern zend_class_entry *mongo_ce_ProtocolException;
extern zend_class_entry *mongo_ce_WriteConcernException;
extern zend_class_entry *mongo_ce_DuplicateKeyException;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

static int php_mongo_api_raise_exception_on_write_failure(mongo_connection *connection, zval *document TSRMLS_DC);
static void php_mongo_api_add_write_options(mongo_buffer *buf, php_mongodb_write_options *write_options TSRMLS_DC);
static void php_mongo_api_throw_exception(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC);
static void php_mongo_api_throw_exception_from_server_code(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC);

/* Converts a zval of $options (passed to ->insert(), ->update(), ...) and creates a
 * php_mongodb_write_options from it */
void php_mongo_api_write_options_from_zval(php_mongodb_write_options *write_options, zval *z_write_options) /* {{{ */
{
	zval **ordered, **fsync, **j, **wtimeout, **w;

	if (!z_write_options || Z_TYPE_P(z_write_options) != IS_ARRAY) {
		return;
	}

	if (zend_hash_find(Z_ARRVAL_P(z_write_options), "ordered", strlen("ordered") + 1, (void**)&ordered) == SUCCESS ) {
		write_options->ordered = zend_is_true(*ordered);
	}

	if (zend_hash_find(Z_ARRVAL_P(z_write_options), "fsync", strlen("fsync") + 1, (void**)&fsync) == SUCCESS) {
		write_options->fsync = zend_is_true(*fsync);
	}

	if (zend_hash_find(Z_ARRVAL_P(z_write_options), "j", strlen("j") + 1, (void**)&j) == SUCCESS) {
		write_options->j = zend_is_true(*j);
	}

	if (zend_hash_find(Z_ARRVAL_P(z_write_options), "wtimeout", strlen("wtimeout") + 1, (void**)&wtimeout) == SUCCESS) {
		convert_to_long_ex(wtimeout);
		write_options->wtimeout = Z_LVAL_PP(wtimeout);
	}

	if (zend_hash_find(Z_ARRVAL_P(z_write_options), "w", strlen("w") + 1, (void**)&w) == SUCCESS) {
		if (Z_TYPE_PP(w) == IS_LONG || Z_TYPE_PP(w) == IS_BOOL) {
			write_options->write_concern.w = Z_LVAL_PP(w);
			write_options->wtype = 1;
		} else {
			convert_to_string_ex(w);
			write_options->write_concern.wstring = Z_STRVAL_PP(w);
			write_options->wtype = 2;
		}
	}
} /* }}} */

/* Converts a php_mongodb_write_options to a zval representation of the structure */
void php_mongo_api_write_options_to_zval(php_mongodb_write_options *write_options, zval *z_write_options) /* {{{ */
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

} /* }}} */

/* Bootstraps a Write API Insert message with its write concerns.
 * Returns the position of the root element, needed to backtract and serialize
 * the size of the combined command */
int php_mongo_api_insert_start(mongo_buffer *buf, char *ns, char *collection, php_mongodb_write_options *write_options TSRMLS_DC) /* {{{ */
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


	/* insert: databasename.collectionName */
	php_mongo_set_type(buf, BSON_STRING);
	php_mongo_serialize_key(buf, "insert", strlen("insert"), 0 TSRMLS_CC);
	php_mongo_serialize_int(buf, strlen(collection) + 1);
	php_mongo_serialize_string(buf, collection, strlen(collection));


	if (write_options) {
		php_mongo_api_add_write_options(buf, write_options TSRMLS_DC);
	}

	/* documents: [ 0: {document: 1}, 1: {document: 2}, ...] */
	php_mongo_set_type(buf, BSON_ARRAY);
	php_mongo_serialize_key(buf, "documents", strlen("documents"), 0 TSRMLS_CC);


	return container_pos;
}
/* }}}  */

/* Adds a new document to write. Must be called after php_mongo_api_insert_start(), and
 * can be called many many times.
 * n: The index of the document (eg: n=0 for first document, n=1 for second doc in a batch)
 * document: The actual document to insert
 * max_document_size: The max document size allowed
 *
 * Will add _id to the document if not present.
 * Throws mongo_ce_Exception on failure (not UTF-8, \0 in key.., overflow max_document_size)
 *
 * NOTE: It is the callers responsibility to "unwind" the document from buf if it wants to
 * keep the existing buffer and ship it without the new document */
int php_mongo_api_insert_add(mongo_buffer *buf, int n, HashTable *document, int max_document_size TSRMLS_DC) /* {{{  */
{
	int docstart = buf->pos-buf->start;
	char *number;

	buf->pos += INT_32;
	php_mongo_set_type(buf, BSON_OBJECT);

	spprintf(&number, 0, "%d", n);
	php_mongo_serialize_key(buf, number, strlen(number), 0 TSRMLS_CC);
	efree(number);

	/* PREP=add _id if it doesn't exist */
	if (zval_to_bson(buf, document, PREP, max_document_size TSRMLS_CC) == FAILURE) {
		return 0;
	}

	php_mongo_serialize_null(buf);
	if (php_mongo_serialize_size(buf->start + docstart, buf, MAX_BSON_WIRE_OBJECT_SIZE(max_document_size) TSRMLS_CC) == FAILURE) {
		return 0;
	}

	return 1;
}
/* }}}  */

/* Finalize the BSON buffer.
 * Requires the container_pos from php_mongo_api_insert_start() and the max_write_size
 * Use MAX_BSON_WIRE_OBJECT_SIZE(max_bson_size) to calculate the correct max_write_size
 * Returns the the full message length.
 * Throws mongo_ce_Exception if the buffer is larger then max_write_size */
int php_mongo_api_insert_end(mongo_buffer *buf, int container_pos, int max_write_size TSRMLS_DC) /* {{{  */
{
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
int php_mongo_api_insert_single(mongo_buffer *buf, char *ns, char *collection, zval *document, php_mongodb_write_options *write_options, mongo_connection *connection TSRMLS_DC) /* {{{  */
{
	int request_id;
	int container_pos;
	int message_lenth;

	request_id = MonGlo(request_id);
	container_pos = php_mongo_api_insert_start(buf, ns, collection, write_options TSRMLS_CC);

	if (!php_mongo_api_insert_add(buf, 0, HASH_OF(document), connection->max_bson_size TSRMLS_CC)) {
		return 0;
	}

	message_lenth = php_mongo_api_insert_end(buf, container_pos, MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size) TSRMLS_CC);
	/* Overflowed the max_write_size */
	if (message_lenth == 0) {
		return 0;
	}

#if MONGO_PHP_STREAMS
	mongo_log_stream_cmd_insert(connection, document, write_options, message_lenth, request_id, ns TSRMLS_CC);
#endif

	return request_id;
}
/* }}}  */


/* Returns 0 on success
 * Returns 1 on failure, and sets EG(exception) */
int php_mongo_api_get_reply(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *options, int socket_read_timeout, int request_id, zval **retval TSRMLS_DC) /* {{{  */
{
	int                status = 0;
	int                data_len = 0;
	char               buf[REPLY_HEADER_SIZE];
	char              *data;
	char              *error_message;
	mongo_msg_header   msg_header;
	php_mongodb_reply  dbreply;


	status = manager->recv_header(connection, options, socket_read_timeout, buf, REPLY_HEADER_LEN, &error_message);
	if (status < 0) {
		/* Read failed, error message populated by recv_header */
		php_mongo_api_throw_exception(connection, abs(status), error_message, NULL TSRMLS_CC);
		return 1;
	} else if (status < INT_32*4) {
		spprintf(&error_message, 256, "couldn't get full response header, got %d bytes but expected atleast %d", status, INT_32*4);
		php_mongo_api_throw_exception(connection, 4, error_message, NULL TSRMLS_CC);
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


#if MONGO_PHP_STREAMS
	mongo_log_stream_write_reply(connection, &msg_header, &dbreply TSRMLS_CC);
#endif

	if (msg_header.length < REPLY_HEADER_SIZE) {
		spprintf(&error_message, 256, "bad response length: %d, did the db assert?", msg_header.length);
		php_mongo_api_throw_exception(connection, 6, error_message, NULL TSRMLS_CC);
		return 1;
	}
	if (msg_header.length > MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size)) {
		spprintf(&error_message, 0, "Message size (%d) overflows valid message size (%d)", msg_header.length, MAX_BSON_WIRE_OBJECT_SIZE(connection->max_bson_size));
		php_mongo_api_throw_exception(connection, 7, error_message, NULL TSRMLS_CC);
		return 1;
	}
	if (msg_header.response_to != request_id) {
		spprintf(&error_message, 0, "request/response mismatch: %d vs %d", request_id, msg_header.response_to);
		php_mongo_api_throw_exception(connection, 9, error_message, NULL TSRMLS_CC);
		return 1;
	}

	data_len = msg_header.length - REPLY_HEADER_LEN;
	data = (char*)emalloc(data_len);

	manager->recv_data(connection, options, 0, data, data_len, &error_message);

	array_init(*retval);
	bson_to_zval(data, Z_ARRVAL_PP(retval), 0 TSRMLS_CC);
	efree(data);

	if (php_mongo_api_raise_exception_on_write_failure(connection, *retval TSRMLS_CC)) {
		return 1;
	}

	return 0;
}
/* }}} */


/* Internal helper.. Writes the php_mongodb_write_options options to the buffer */
static void php_mongo_api_add_write_options(mongo_buffer *buf, php_mongodb_write_options *write_options TSRMLS_DC) /* {{{  */
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

/* Internal helper.. Raises an exception if needed
 * Returns 0 when write succeeded (ok=1)
 * Returns 1 when write failed, and raises different exceptions depending on the error */
static int php_mongo_api_raise_exception_on_write_failure(mongo_connection *connection, zval *document TSRMLS_DC) /* {{{ */
{
	zval **ok, **code, **errmsg;

	if (zend_hash_find(Z_ARRVAL_P(document), "ok", strlen("ok") + 1, (void**)&ok) == SUCCESS) {
		convert_to_boolean(*ok);
		if (Z_BVAL_PP(ok)) {
			/* Write succeeded */
			return 0;
		}
	} else {
		/* Missing OK field... that can't be good! */
		php_mongo_api_throw_exception_from_server_code(connection, 100, "Missing 'ok' field in response, don't know what to do", document TSRMLS_CC);
		return 1;
	}

	if (zend_hash_find(Z_ARRVAL_P(document), "code", strlen("code") + 1, (void**)&code) == SUCCESS) {
		convert_to_long(*code);

		if (zend_hash_find(Z_ARRVAL_P(document), "errmsg", strlen("errmsg") + 1, (void**)&errmsg) == SUCCESS) {
			convert_to_string(*errmsg);

			php_mongo_api_throw_exception_from_server_code(connection, Z_LVAL_PP(code), Z_STRVAL_PP(errmsg), document TSRMLS_CC);
			return 1;
		}

		php_mongo_api_throw_exception_from_server_code(connection, Z_LVAL_PP(code), "Unknown failure, no error message from server", document TSRMLS_CC);
		return 1;
	}

	/* Couldn't find a error code.. do I atleast have a error message? */
	if (zend_hash_find(Z_ARRVAL_P(document), "errmsg", strlen("errmsg") + 1, (void**)&errmsg) == SUCCESS) {
		convert_to_string(*errmsg);

		php_mongo_api_throw_exception_from_server_code(connection, 0, Z_STRVAL_PP(errmsg), document TSRMLS_CC);
		return 1;
	}

	php_mongo_api_throw_exception_from_server_code(connection, 0, "Unknown error occurred, did not get an error message or code", document TSRMLS_CC);
	return 1;
} /* }}} */

/* Internal helper.. raises exception based on the server error code */
static void php_mongo_api_throw_exception_from_server_code(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC) /* {{{ */
{
	zval *exception;
	zend_class_entry *ce;

	switch(code) {
	case 11000: /* Duplicate key Exception */
		ce = mongo_ce_DuplicateKeyException;
		break;

	default: /* Any other error we would have hit */
		ce = mongo_ce_WriteConcernException;
		break;

	}

	exception = mongo_cursor_throw(ce, connection, code TSRMLS_CC, "%s", error_message);

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

/* Internal helper.. raises exception based on our failure codes */
static void php_mongo_api_throw_exception(mongo_connection *connection, int code, char *error_message, zval *document TSRMLS_DC) /* {{{ */
{
	zval *exception;
	zend_class_entry *ce;

	switch(code) {
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
	case 7: /* Message size (%d) overflows valid message size (%d) php_mongo_api_get_reply() */
	case 9: /* request/response mismatch: %d vs %d php_mongo_api_get_reply() */
		ce = mongo_ce_CursorException; /* should be MongoProtocol error php_mongo_api_get_reply() */
		break;

	default:
		ce = mongo_ce_ProtocolException;
		break;

	}

	exception = mongo_cursor_throw(ce, connection, code TSRMLS_CC, "%s", error_message);

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
