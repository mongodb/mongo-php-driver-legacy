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

#include "stream.h"
#include "types.h"

#include "php.h"
#include "config.h"
#include "/Users/bjori/.apps/5.4/include/php/main/php_streams.h"
#include "/Users/bjori/.apps/5.4/include/php/main/php_network.h"


php_stream* php_mongo_stream_connect(mongo_con_manager *manager, mongo_server_options *options, mongo_server_def *server, char **error_message)
{
	int errcode;
	char *errstr;
	const char *mode = strdup("rwb");
	char *persistent_id = NULL;
	php_stream *stream = php_stream_xport_create("sslv3://localhost:27017", strlen("sslv3://localhost:27017"), 0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, NULL, NULL, NULL, &errstr, &errcode);
	php_stream_context *context = php_stream_context_alloc(TSRMLS_C);
	/*
	stream = php_stream_alloc_rel(&php_openssl_socket_ops, sslsock, persistent_id, "r+")

	stream = php_stream_fopen_from_fd_rel(tmp_socket, mode, persistent_id);
	{
		php_openssl_netstream_data_t sslstream = (php_openssl_netstream_data_t *)stream->abstract;
		stream->abstract = sslstream;
	}
	*/


	if (!stream) {
		printf("ERROR: %s (%d)", errstr, errcode);
		exit(42);
	}
	//php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_TLS_CLIENT, NULL TSRMLS_CC);
	php_stream_xport_crypto_enable(stream, 1 TSRMLS_CC);
	/*
	php_stream_context_set(stream, context);
	if (php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_TLS_CLIENT, NULL TSRMLS_CC) < 0 ||
		printf("Failed setting up stuff");
	} else {
		printf("Everything cool");
	}

	php_stream_context_set(stream, NULL);
	*/
	return stream;
	/*
	{
		php_netstream_data_t *sock = (php_openssl_netstream_data_t*)stream->abstract;
		return sock->s.socket;
	}
	*/
}
int php_mongo_stream_read(mongo_connection *con, mongo_server_options *options, void *data, int size, char **error_message)
{
	int retval = php_stream_read(con->socket, (char *) data, size);
	printf("READ: Got retval: %d\nData:%s\nsize: %d\n\n", retval, data, size);

	return retval;
}
int php_mongo_stream_write(mongo_connection *con, mongo_server_options *options, void *data, int size, char **error_message)
{
	int retval =  php_stream_write(con->socket, (char *) data, size);
	printf("WRITE: Got retval: %d\nData:%s\nsize: %d\n\n", retval, data, size);

	return retval;
}



