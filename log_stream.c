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

#include "log_stream.h"
#include "io_stream.h"
#include "mcon/types.h"
#include "mcon/utils.h"
#include "mcon/manager.h"
#include "php_mongo.h"
#include "bson.h"
#include "cursor_shared.h"

#include <php.h>
#include <main/php_streams.h>
#include <main/php_network.h>
#include <ext/standard/php_smart_str.h>
#include <ext/standard/php_var.h>
#include <ext/standard/basic_functions.h>


#define ADD_ASSOC_ZVAL_ADDREF(args, key, zval) Z_ADDREF_P(zval); add_assoc_zval(args, key, zval)
#define CONTEXT_HAS_NOTIFY_OR_LOG(context, method) (context && (php_stream_context_get_option(context, "mongodb", #method, NULL) || context->notifier))

void php_mongo_stream_notify_meta(php_stream_context *ctx, int code, zval *args TSRMLS_DC)
{
	if (ctx && ctx->notifier) {
		smart_str buf = {NULL, 0, 0};
		php_serialize_data_t var_hash;

#if PHP_VERSION_ID >= 50400
		/* Workaround for a seemingly llvm bug?
		 * Can only reproduce this with PHP 5.5 (like 5.5.3) and MacOSX
		 * Apple clang version 3.1 (tags/Apple/clang-318.0.61) (based on LLVM 3.1svn)
		 * If you dump the value from a debugger it will be 0, but its always interperited as
		 * the max value of unsigned int for magical reasons
		 */
		if (BG(serialize).level == -1) {
			BG(serialize).level = 0;
		}
#endif
		PHP_VAR_SERIALIZE_INIT(var_hash);
		php_var_serialize(&buf, &args, &var_hash TSRMLS_CC);
		PHP_VAR_SERIALIZE_DESTROY(var_hash);

		buf.c[buf.len] = 0;
		php_stream_notification_notify(ctx, MONGO_STREAM_NOTIFY_TYPE_LOG, PHP_STREAM_NOTIFY_SEVERITY_INFO, buf.c, code, 0, 0, NULL TSRMLS_CC);
		smart_str_free(&buf);
	}
}

void php_mongo_stream_notify_meta_write_batch(php_stream_context *ctx, zval *server, zval *write_options, zval *batch, zval *protocol_options TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "batch_retval", batch);
	ADD_ASSOC_ZVAL_ADDREF(args, "write_options", write_options);
	ADD_ASSOC_ZVAL_ADDREF(args, "protocol_options", protocol_options);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_WRITE_BATCH, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_cmd_delete(php_stream_context *ctx, zval *server, zval *write_options, zval *delete_options, zval *protocol_options TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "delete_options", delete_options);
	ADD_ASSOC_ZVAL_ADDREF(args, "write_options", write_options);
	ADD_ASSOC_ZVAL_ADDREF(args, "protocol_options", protocol_options);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_CMD_DELETE, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_cmd_update(php_stream_context *ctx, zval *server, zval *write_options, zval *update_options, zval *protocol_options TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "update_options", update_options);
	ADD_ASSOC_ZVAL_ADDREF(args, "write_options", write_options);
	ADD_ASSOC_ZVAL_ADDREF(args, "protocol_options", protocol_options);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_CMD_UPDATE, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_cmd_insert(php_stream_context *ctx, zval *server, zval *document, zval *write_options, zval *protocol_options TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "document", document);
	ADD_ASSOC_ZVAL_ADDREF(args, "write_options", write_options);
	ADD_ASSOC_ZVAL_ADDREF(args, "protocol_options", protocol_options);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_CMD_INSERT, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_insert(php_stream_context *ctx, zval *server, zval *document, zval *options TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "document", document);
	ADD_ASSOC_ZVAL_ADDREF(args, "options", options);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_INSERT, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_query(php_stream_context *ctx, zval *server, zval *query, zval *info TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "query", query);
	ADD_ASSOC_ZVAL_ADDREF(args, "info", info);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_QUERY, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_update(php_stream_context *ctx, zval *server, zval *criteria, zval *newobj, zval *options, zval *info TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "criteria", criteria);
	ADD_ASSOC_ZVAL_ADDREF(args, "newobj", newobj);
	ADD_ASSOC_ZVAL_ADDREF(args, "options", options);
	ADD_ASSOC_ZVAL_ADDREF(args, "info", info);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_UPDATE, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_delete(php_stream_context *ctx, zval *server, zval *criteria, zval *options, zval *info TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "criteria", criteria);
	ADD_ASSOC_ZVAL_ADDREF(args, "options", options);
	ADD_ASSOC_ZVAL_ADDREF(args, "info", info);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_DELETE, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_getmore(php_stream_context *ctx, zval *server, zval *info TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "info", info);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_GETMORE, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_killcursor(php_stream_context *ctx, zval *server, zval *info TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "info", info);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_KILLCURSOR, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_batchinsert(php_stream_context *ctx, zval *server, zval *docs, zval *options, zval *info TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "docs", docs);
	ADD_ASSOC_ZVAL_ADDREF(args, "options", options);
	ADD_ASSOC_ZVAL_ADDREF(args, "info", info);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_BATCHINSERT, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_response_header(php_stream_context *ctx, zval *server, zval *query, zval *info TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "query", query);
	ADD_ASSOC_ZVAL_ADDREF(args, "info", info);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_RESPONSE_HEADER, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_meta_write_reply(php_stream_context *ctx, zval *server, zval *op, zval *reply TSRMLS_DC)
{
	zval *args;

	MAKE_STD_ZVAL(args);
	array_init(args);

	ADD_ASSOC_ZVAL_ADDREF(args, "server", server);
	ADD_ASSOC_ZVAL_ADDREF(args, "op", op);
	ADD_ASSOC_ZVAL_ADDREF(args, "reply", reply);

	php_mongo_stream_notify_meta(ctx, MONGO_STREAM_NOTIFY_LOG_WRITE_REPLY, args TSRMLS_CC);
	zval_ptr_dtor(&args);
}

void php_mongo_stream_notify_io(php_stream *stream, int code, int sofar, int max TSRMLS_DC)
{
	php_stream_context *ctx = stream->context;

	if (!ctx || !ctx->notifier) {
		return;
	}

	switch (code) {
		case MONGO_STREAM_NOTIFY_IO_READ:
		case MONGO_STREAM_NOTIFY_IO_WRITE:
			ctx->notifier->progress = 0;
			ctx->notifier->progress_max = max;
			ctx->notifier->mask |= PHP_STREAM_NOTIFIER_PROGRESS;
			php_stream_notification_notify(ctx, MONGO_STREAM_NOTIFY_TYPE_IO_INIT, PHP_STREAM_NOTIFY_SEVERITY_INFO, NULL, code, 0, max, NULL TSRMLS_CC);
			break;
		case MONGO_STREAM_NOTIFY_IO_COMPLETED:
		case MONGO_STREAM_NOTIFY_IO_PROGRESS:
			php_stream_notification_notify(ctx, code, PHP_STREAM_NOTIFY_SEVERITY_INFO, NULL, 0, sofar, max, NULL TSRMLS_CC);
			break;
	}
}

zval *php_log_get_server_info(mongo_connection *connection)
{
	zval *retval;
	MAKE_STD_ZVAL(retval);
	array_init(retval);

	add_assoc_string(retval, "hash", connection->hash, 1);
	add_assoc_long(retval, "type", connection->connection_type);
	add_assoc_long(retval, "max_bson_size", connection->max_bson_size);
	add_assoc_long(retval, "max_message_size", connection->max_message_size);
	add_assoc_long(retval, "request_id", connection->last_reqid);

	return retval;
}

void php_mongo_stream_callback(php_stream_context *ctx, char *cb_name, int argc, zval **args[] TSRMLS_DC)
{
	zval **callback;
	zval *retval = NULL;

	if (php_stream_context_get_option(ctx, "mongodb", cb_name, &callback) == SUCCESS) {
		if (FAILURE == call_user_function_ex(EG(function_table), NULL, *callback, &retval, argc, args, 0, NULL TSRMLS_CC)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "failed to call stream context callback function '%s' for 'mongodb' context option", cb_name);
		}
	}

	if (retval) {
		zval_ptr_dtor(&retval);
	}
}

void mongo_log_stream_insert(mongo_connection *connection, zval *document, zval *options TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_insert")) {
		zval **args[3];
		zval *server;
		int   free_options = 0;

		server = php_log_get_server_info(connection);

		args[0] = &server;
		args[1] = &document;
		if (!options) {
			free_options = 1;
			MAKE_STD_ZVAL(options);
			ZVAL_NULL(options);
		}
		args[2] = &options;

		php_mongo_stream_notify_meta_insert(context, server, document, options TSRMLS_CC);
		php_mongo_stream_callback(context, "log_insert", 3, args TSRMLS_CC);

		zval_ptr_dtor(&server);
		if (free_options) {
			zval_ptr_dtor(args[2]);
		}
	}
}

void mongo_log_stream_query(mongo_connection *connection, mongo_cursor *cursor TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_query")) {
		zval **args[3];
		zval *server, *info;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(info);
		array_init(info);

		add_assoc_long(info, "request_id", cursor->send.request_id);
		add_assoc_long(info, "skip", cursor->skip);
		add_assoc_long(info, "limit", cursor->limit);
		add_assoc_long(info, "options", cursor->opts);
		add_assoc_long(info, "cursor_id", cursor->cursor_id);
		add_assoc_string(info, "ns", cursor->ns, 1);

		args[0] = &server;
		args[1] = &cursor->query;
		args[2] = &info;


		php_mongo_stream_notify_meta_query(context, server, cursor->query, info TSRMLS_CC);
		php_mongo_stream_callback(context, "log_query", 3, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		zval_ptr_dtor(&info);
	}
}

void mongo_log_stream_update(mongo_connection *connection, zval *ns, zval *criteria, zval *newobj, zval *options, int flags TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_update")) {
		zval **args[5];
		zval *server, *info;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(info);
		array_init(info);

		add_assoc_stringl(info, "namespace", Z_STRVAL_P(ns), Z_STRLEN_P(ns), 1);
		add_assoc_long(info, "flags", flags);

		args[0] = &server;
		args[1] = &criteria;
		args[2] = &newobj;
		args[3] = &options;
		args[4] = &info;


		php_mongo_stream_notify_meta_update(context, server, criteria, newobj, options, info TSRMLS_CC);
		php_mongo_stream_callback(context, "log_update", 5, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		zval_ptr_dtor(&info);
	}
}

void mongo_log_stream_delete(mongo_connection *connection, zval *ns, zval *criteria, zval *options, int flags TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_delete")) {
		zval **args[4];
		zval *server, *info;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(info);
		array_init(info);

		add_assoc_stringl(info, "namespace", Z_STRVAL_P(ns), Z_STRLEN_P(ns), 1);
		add_assoc_long(info, "flags", flags);

		args[0] = &server;
		args[1] = &criteria;
		args[2] = &options;
		args[3] = &info;


		php_mongo_stream_notify_meta_delete(context, server, criteria, options, info TSRMLS_CC);
		php_mongo_stream_callback(context, "log_delete", 4, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		zval_ptr_dtor(&info);
	}
}

void mongo_log_stream_getmore(mongo_connection *connection, mongo_cursor *cursor TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_getmore")) {
		zval **args[2];
		zval *server, *info;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(info);
		array_init(info);

		add_assoc_long(info, "request_id", cursor->recv.request_id);
		add_assoc_long(info, "cursor_id", cursor->cursor_id);
		add_assoc_long(info, "batch_size", cursor->batch_size);

		args[0] = &server;
		args[1] = &info;


		php_mongo_stream_notify_meta_getmore(context, server, info TSRMLS_CC);
		php_mongo_stream_callback(context, "log_getmore", 2, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		zval_ptr_dtor(&info);
	}
}

void mongo_log_stream_killcursor(mongo_connection *connection, int64_t cursor_id TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_killcursor")) {
		zval **args[2];
		zval *server, *info;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(info);
		array_init(info);

		add_assoc_long(info, "cursor_id", cursor_id);

		args[0] = &server;
		args[1] = &info;


		php_mongo_stream_notify_meta_killcursor(context, server, info TSRMLS_CC);
		php_mongo_stream_callback(context, "log_killcursor", 2, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		zval_ptr_dtor(&info);
	}
}

void mongo_log_stream_batchinsert(mongo_connection *connection, zval *docs, zval *options, int flags TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_batchinsert")) {
		zval **args[4];
		zval *server, *info;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(info);
		array_init(info);

		add_assoc_long(info, "flags", flags);


		args[0] = &server;
		args[1] = &docs;
		args[2] = &options;
		args[3] = &info;

		php_mongo_stream_notify_meta_batchinsert(context, server, docs, options, info TSRMLS_CC);
		php_mongo_stream_callback(context, "log_batchinsert", 4, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		zval_ptr_dtor(&info);
	}
}

void mongo_log_stream_response_header(mongo_connection *connection, mongo_cursor *cursor TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_response_header")) {
		zval **args[3];
		zval *server, *query, *info;
		int free_query = 0;

		server = php_log_get_server_info(connection);

		/* Command cursors created from a command result document will lack the
		 * original query document. Allocate a null zval in that case. */
		if (cursor->query) {
			query = cursor->query;
		} else {
			MAKE_STD_ZVAL(query);
			ZVAL_NULL(query);
			free_query = 1;
		}

		MAKE_STD_ZVAL(info);
		array_init(info);

		add_assoc_long(info, "send_request_id", cursor->send.request_id);
		add_assoc_long(info, "cursor_id", cursor->cursor_id);
		add_assoc_long(info, "recv_request_id", cursor->recv.request_id);
		add_assoc_long(info, "recv_response_to", cursor->recv.response_to);
		add_assoc_long(info, "recv_opcode", cursor->recv.op);
		add_assoc_long(info, "flag", cursor->flag);
		add_assoc_long(info, "start", cursor->start);

		args[0] = &server;
		args[1] = cursor->query ? &cursor->query : &query;
		args[2] = &info;

		php_mongo_stream_notify_meta_response_header(context, server, query, info TSRMLS_CC);
		php_mongo_stream_callback(context, "log_response_header", 3, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		if (free_query) {
			zval_ptr_dtor(&query);
		}
		zval_ptr_dtor(&info);
	}
}

void mongo_log_stream_write_reply(mongo_connection *connection, mongo_msg_header *header, php_mongo_reply *reply TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_reply")) {
		zval **args[3];
		zval *server, *zop, *zreply;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(zop);
		array_init(zop);

		MAKE_STD_ZVAL(zreply);
		array_init(zreply);

		add_assoc_long(zop, "length", header->length);
		add_assoc_long(zop, "request_id", header->request_id);
		add_assoc_long(zop, "response_to", header->response_to);
		add_assoc_long(zop, "op", header->op);
		add_assoc_long(zreply, "flags", reply->flags);
		add_assoc_long(zreply, "cursor_id", reply->cursor_id);
		add_assoc_long(zreply, "start", reply->start);
		add_assoc_long(zreply, "returned", reply->returned);

		args[0] = &server;
		args[1] = &zop;
		args[2] = &zreply;


		php_mongo_stream_notify_meta_write_reply(context, server, zop, zreply TSRMLS_CC);
		php_mongo_stream_callback(context, "log_reply", 3, args TSRMLS_CC);
		zval_ptr_dtor(&server);
		zval_ptr_dtor(&zop);
		zval_ptr_dtor(&zreply);
	}
}

void mongo_log_stream_cmd_insert(mongo_connection *connection, zval *document, php_mongo_write_options *write_options, int message_length, int request_id, char *ns TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_cmd_insert")) {
		zval **args[4];
		zval *protocol_options, *server, *z_write_options;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(protocol_options);
		array_init(protocol_options);

		MAKE_STD_ZVAL(z_write_options);
		array_init(z_write_options);

		php_mongo_api_write_options_to_zval(write_options, z_write_options);

		add_assoc_long(protocol_options, "message_length", message_length);
		add_assoc_long(protocol_options, "request_id", request_id);
		add_assoc_stringl(protocol_options, "namespace", ns, strlen(ns), 1);

		args[0] = &server;
		args[1] = &document;
		args[2] = &z_write_options;
		args[3] = &protocol_options;

		php_mongo_stream_notify_meta_cmd_insert(context, server, document, z_write_options, protocol_options TSRMLS_CC);
		php_mongo_stream_callback(context, "log_cmd_insert", 4, args TSRMLS_CC);

		zval_ptr_dtor(&server);
		zval_ptr_dtor(&protocol_options);
		zval_ptr_dtor(&z_write_options);
	}
}

void php_update_options_to_zval(php_mongo_write_update_args *update_options, zval *z_update_options)
{
	add_assoc_bool(z_update_options, "multi", update_options->multi);
	add_assoc_bool(z_update_options, "upsert", update_options->upsert);
	ADD_ASSOC_ZVAL_ADDREF(z_update_options, "q", update_options->query);
	ADD_ASSOC_ZVAL_ADDREF(z_update_options, "u", update_options->update);
}

void mongo_log_stream_cmd_update(mongo_connection *connection, php_mongo_write_update_args *update_options, php_mongo_write_options *write_options, int message_length, int request_id, char *ns TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_cmd_update")) {
		zval **args[4];
		zval *protocol_options, *server, *z_write_options, *z_update_options;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(protocol_options);
		array_init(protocol_options);

		MAKE_STD_ZVAL(z_write_options);
		array_init(z_write_options);

		MAKE_STD_ZVAL(z_update_options);
		array_init(z_update_options);

		php_mongo_api_write_options_to_zval(write_options, z_write_options);
		php_update_options_to_zval(update_options, z_update_options);

		add_assoc_long(protocol_options, "message_length", message_length);
		add_assoc_long(protocol_options, "request_id", request_id);
		add_assoc_stringl(protocol_options, "namespace", ns, strlen(ns), 1);

		args[0] = &server;
		args[1] = &z_write_options;
		args[2] = &z_update_options;
		args[3] = &protocol_options;

		php_mongo_stream_notify_meta_cmd_update(context, server, z_write_options, z_update_options, protocol_options TSRMLS_CC);
		php_mongo_stream_callback(context, "log_cmd_update", 4, args TSRMLS_CC);

		zval_ptr_dtor(&server);
		zval_ptr_dtor(&protocol_options);
		zval_ptr_dtor(&z_write_options);
		zval_ptr_dtor(&z_update_options);
	}
}

void php_delete_options_to_zval(php_mongo_write_delete_args *delete_options, zval *z_delete_options)
{
	add_assoc_long(z_delete_options, "limit", delete_options->limit);
	ADD_ASSOC_ZVAL_ADDREF(z_delete_options, "q", delete_options->query);
}

void mongo_log_stream_cmd_delete(mongo_connection *connection, php_mongo_write_delete_args *delete_options, php_mongo_write_options *write_options, int message_length, int request_id, char *ns TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_cmd_delete")) {
		zval **args[4];
		zval *protocol_options, *server, *z_write_options, *z_delete_options;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(protocol_options);
		array_init(protocol_options);

		MAKE_STD_ZVAL(z_write_options);
		array_init(z_write_options);

		MAKE_STD_ZVAL(z_delete_options);
		array_init(z_delete_options);

		php_mongo_api_write_options_to_zval(write_options, z_write_options);
		php_delete_options_to_zval(delete_options, z_delete_options);

		add_assoc_long(protocol_options, "message_length", message_length);
		add_assoc_long(protocol_options, "request_id", request_id);
		add_assoc_stringl(protocol_options, "namespace", ns, strlen(ns), 1);

		args[0] = &server;
		args[1] = &z_write_options;
		args[2] = &z_delete_options;
		args[3] = &protocol_options;

		php_mongo_stream_notify_meta_cmd_delete(context, server, z_write_options, z_delete_options, protocol_options TSRMLS_CC);
		php_mongo_stream_callback(context, "log_cmd_delete", 4, args TSRMLS_CC);

		zval_ptr_dtor(&server);
		zval_ptr_dtor(&protocol_options);
		zval_ptr_dtor(&z_write_options);
		zval_ptr_dtor(&z_delete_options);
	}
}

void mongo_log_stream_write_batch(mongo_connection *connection, php_mongo_write_options *write_options, int request_id, zval *batch TSRMLS_DC)
{
	php_stream_context *context = ((php_stream *)connection->socket)->context;

	if (CONTEXT_HAS_NOTIFY_OR_LOG(context, "log_write_batch")) {
		zval **args[4];
		zval *protocol_options, *server, *z_write_options;

		server = php_log_get_server_info(connection);

		MAKE_STD_ZVAL(protocol_options);
		array_init(protocol_options);

		MAKE_STD_ZVAL(z_write_options);
		array_init(z_write_options);

		php_mongo_api_write_options_to_zval(write_options, z_write_options);

		add_assoc_long(protocol_options, "request_id", request_id);

		args[0] = &server;
		args[1] = &z_write_options;
		args[2] = &batch;
		args[3] = &protocol_options;

		php_mongo_stream_notify_meta_write_batch(context, server, z_write_options, batch, protocol_options TSRMLS_CC);
		php_mongo_stream_callback(context, "log_write_batch", 4, args TSRMLS_CC);

		zval_ptr_dtor(&server);
		zval_ptr_dtor(&protocol_options);
		zval_ptr_dtor(&z_write_options);
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
