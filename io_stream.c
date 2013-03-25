/**
 *  Copyright 2009-2013 10gen, Inc.
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

#include "io_stream.h"
#include "mcon/types.h"
#include "mcon/utils.h"
#include "mcon/manager.h"
#include "php_mongo.h"

#include "php.h"
#include "config.h"
#include "main/php_streams.h"
#include "main/php_network.h"


void* php_mongo_io_stream_connect(mongo_con_manager *manager, mongo_server_def *server, mongo_server_options *options, char **error_message)
{
	char *errmsg;
	int errcode;
	php_stream *stream;
	char *hash = mongo_server_create_hash(server);
	struct timeval ctimeout = {0};
	char *dsn;
	int dsn_len;
	TSRMLS_FETCH();

	dsn_len = spprintf(&dsn, 0, "tcp://%s:%d", server->host, server->port);

	if (options->connectTimeoutMS) {
		ctimeout.tv_sec = options->connectTimeoutMS / 1000;
		ctimeout.tv_usec = (options->connectTimeoutMS % 1000) * 1000;
	}

	stream = php_stream_xport_create(dsn, dsn_len + 1, 0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, hash, options->connectTimeoutMS ? &ctimeout : NULL, (php_stream_context *)options->ctx, &errmsg, &errcode);
	efree(dsn);
	free(hash);

	if (!stream) {
		/* error_message will be free()d, but errmsg was allocated by PHP and needs efree() */
		*error_message = strdup(errmsg);
		efree(errmsg);
		return NULL;
	}

	if (options->ssl) {
		if (
			(php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_TLS_CLIENT, NULL TSRMLS_CC) < 0) ||
			(php_stream_xport_crypto_enable(stream, 1 TSRMLS_CC) < 0))
		{
			/* Setting up crypto failed. Thats only OK if we only preferred it */
			if (options->ssl == MONGO_SSL_PREFER) {
				/* FIXME: We can't actually get here because we reject setting this optino to prefer in mcon/parse.c
				 * This is however probably what we need to do in the future when mongod starts actually supporting this! :)
				 */
				mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "stream_connect: Failed establishing SSL for %s:%d", server->host, server->port);
				php_stream_xport_crypto_enable(stream, 0 TSRMLS_CC);
			} else {
				*error_message = strdup("Can't connect over SSL, is mongod running with SSL?");
				php_stream_close(stream);
				return NULL;
			}
		} else {
			mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "stream_connect: Establish SSL for %s:%d", server->host, server->port);
		}
	} else {
		mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "stream_connect: Not establishing SSL for %s:%d", server->host, server->port);
	}

	php_stream_notify_progress_init(stream->context, 0, 0);

	if (options->socketTimeoutMS) {
		struct timeval rtimeout = {0};
		rtimeout.tv_sec = options->socketTimeoutMS / 1000;
		rtimeout.tv_usec = (options->socketTimeoutMS % 1000) * 1000;
		php_stream_set_option(stream, PHP_STREAM_OPTION_READ_TIMEOUT, 0, &rtimeout);
	}


	/* Avoid a weird leak warning in debug mode when freeing the stream */
#if ZEND_DEBUG
	stream->__exposed = 1;
#endif
	return stream;

}

int php_mongo_io_stream_read(mongo_connection *con, mongo_server_options *options, void *data, int size, char **error_message)
{
	int num = 1, received = 0;
	TSRMLS_FETCH();

	/* this can return FAILED if there is just no more data from db */
	while (received < size && num > 0) {
		int len = 4096 < (size - received) ? 4096 : size - received;

		num = php_stream_read(con->socket, (char *) data, len);

		if (num < 0) {
			return -1;
		}

		data = (char*)data + num;
		received += num;
	}

	if (options && options->ctx) {
		php_stream_notify_progress_increment((php_stream_context *)options->ctx, received, size);
	}

	return received;
}

int php_mongo_io_stream_send(mongo_connection *con, mongo_server_options *options, void *data, int size, char **error_message)
{
	int retval;
	TSRMLS_FETCH();

	retval = php_stream_write(con->socket, (char *) data, size);

	return retval;
}

void php_mongo_io_stream_close(mongo_connection *con, int why)
{
	TSRMLS_FETCH();

	if (why == MONGO_CLOSE_BROKEN) {
		if (con->socket) {
			php_stream_free(con->socket, PHP_STREAM_FREE_CLOSE_PERSISTENT | PHP_STREAM_FREE_RSRC_DTOR);
		}
	} else if (why == MONGO_CLOSE_SHUTDOWN) {
		/* No need to do anything, it was freed from the persistent_list */
	}
}

void php_mongo_io_stream_forget(mongo_con_manager *manager, mongo_connection *con)
{
	zend_rsrc_list_entry *le;
	TSRMLS_FETCH();

	/* When we fork we need to unregister the parents hash so we don't accidentally destroy it */
	if (zend_hash_find(&EG(persistent_list), con->hash, strlen(con->hash) + 1, (void*) &le) == SUCCESS) {
		((php_stream *)con->socket)->in_free = 1;
		zend_hash_del(&EG(persistent_list), con->hash, strlen(con->hash) + 1);
		((php_stream *)con->socket)->in_free = 0;
	}
}
