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

#include "php.h"
#include "config.h"
#include "/Users/bjori/.apps/5.4/include/php/main/php_streams.h"
#include "/Users/bjori/.apps/5.4/include/php/main/php_network.h"


void* php_mongo_io_stream_connect(mongo_server_def *server, mongo_server_options *options, char **error_message)
{
	char *errmsg;
	int errcode;
	const char *mode = "rwb";
	php_stream *stream;
	char *hash = mongo_server_create_hash(server);
	zend_rsrc_list_entry *le;
	struct timeval ctimeout = {0};

	if (options->connectTimeoutMS) {
		ctimeout.tv_sec = options->connectTimeoutMS / 1000;
		ctimeout.tv_usec = (options->connectTimeoutMS % 1000) * 1000;
	}
	
	stream = php_stream_xport_create("tcp://localhost:27017", strlen("tcp://localhost:27017"), 0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, hash, options->connectTimeoutMS ? &ctimeout : NULL, NULL, &errmsg, &errcode);

	free(hash);

	/*
	 * FIXME: STREAMS: When we start supporting certificates, validation and stuff we need stream contexts..
	 * php_stream_context *context = php_stream_context_alloc(TSRMLS_C);
	 * php_stream_context_set(stream, context);
	 *
	 */

	if (!stream) {
		*error_message = strdup(errmsg);
		return NULL;
	}

	if (options->ssl) {
		if (php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_TLS_CLIENT, NULL TSRMLS_CC) < 0
			|| php_stream_xport_crypto_enable(stream, 1 TSRMLS_CC) < 0) {
			/* Setting up crypto failed. Thats only OK if we only preferred it */
			if (options->ssl == MONGO_SSL_PREFER) {
				php_stream_xport_crypto_enable(stream, 0 TSRMLS_CC);
			} else {
				*error_message = strdup("Can't connect over SSL, is mongod running with SSL?");
				php_stream_close(stream);
				return NULL;
			}
		}
	}

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
	int retval = php_stream_read(con->consocket, (char *) data, size);
	printf("READ: Got retval: %d\nData:%s\nsize: %d\n\n", retval, (char *)data, size);

	return retval;
}
int php_mongo_io_stream_send(mongo_connection *con, mongo_server_options *options, void *data, int size, char **error_message)
{
	int retval =  php_stream_write(con->consocket, (char *) data, size);
	printf("WRITE: Got retval: %d\nData:%s\nsize: %d\n\n", retval, (char *)data, size);

	return retval;
}

void php_mongo_io_stream_close(mongo_connection *con, int why)
{
	if (why == MONGO_CLOSE_BROKEN) {
		if (con->consocket) {
			php_stream_free(con->consocket, PHP_STREAM_FREE_CLOSE_PERSISTENT | PHP_STREAM_FREE_RSRC_DTOR);
		}
	} else if (why == MONGO_CLOSE_SHUTDOWN) {
		/* No need to do anything, it was freed from the persistent_list */
		//php_stream_close(con->consocket);
	}
}


