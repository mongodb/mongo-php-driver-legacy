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

#include "io_stream.h"
#include "log_stream.h"
#include "mcon/types.h"
#include "mcon/utils.h"
#include "mcon/manager.h"
#include "mcon/connections.h"
#include "php_mongo.h"

#include <php.h>
#include <main/php_streams.h>
#include <main/php_network.h>
#include <ext/standard/file.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_MONGO_SASL
#include <sasl/sasl.h>
#include <sasl/saslutil.h>
#endif

extern zend_class_entry *mongo_ce_ConnectionException;
ZEND_EXTERN_MODULE_GLOBALS(mongo)

void* php_mongo_io_stream_connect(mongo_con_manager *manager, mongo_server_def *server, mongo_server_options *options, char **error_message)
{
	char *errmsg;
	int errcode;
	php_stream *stream;
	char *hash = mongo_server_create_hash(server);
	struct timeval ctimeout = {0, 0};
	char *dsn;
	int dsn_len;
	int tcp_socket = 1;
	zend_error_handling error_handler;

	TSRMLS_FETCH();

	if (server->host[0] == '/') {
		dsn_len = spprintf(&dsn, 0, "unix://%s", server->host);
		tcp_socket = 0;
	} else {
		dsn_len = spprintf(&dsn, 0, "tcp://%s:%d", server->host, server->port);
	}


	/* Connection timeout behavior varies based on the following:
	 * - Negative => no timeout (i.e. block indefinitely)
	 * - Zero => not specified (PHP will use default_socket_timeout)
	 * - Positive => used specified timeout */
	if (options->connectTimeoutMS) {
		/* Convert negative value to -1 second, which implies no timeout */
		int connectTimeoutMS = options->connectTimeoutMS < 0 ? -1000 : options->connectTimeoutMS;

		ctimeout.tv_sec = connectTimeoutMS / 1000;
		ctimeout.tv_usec = (connectTimeoutMS % 1000) * 1000;
		mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "Connecting to %s (%s) with connection timeout: %d.%06d", dsn, hash, ctimeout.tv_sec, ctimeout.tv_usec);
	} else {
		mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "Connecting to %s (%s) without connection timeout (default_socket_timeout will be used)", dsn, hash);
	}

	zend_replace_error_handling(EH_THROW, mongo_ce_ConnectionException, &error_handler TSRMLS_CC);
	stream = php_stream_xport_create(dsn, dsn_len, 0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, hash, options->connectTimeoutMS > 0 ? &ctimeout : NULL, (php_stream_context *)options->ctx, &errmsg, &errcode);
	zend_restore_error_handling(&error_handler TSRMLS_CC);

	efree(dsn);
	free(hash);

	if (!stream) {
		/* error_message will be free()d, but errmsg was allocated by PHP and needs efree() */
		*error_message = strdup(errmsg);
		efree(errmsg);
		return NULL;
	}

	if (tcp_socket) {
		int socket = ((php_netstream_data_t*)stream->abstract)->socket;
		int flag = 1;

		setsockopt(socket, IPPROTO_TCP,  TCP_NODELAY, (char *) &flag, sizeof(int));
	}

	if (options->ssl) {
		int crypto_enabled;

		zend_replace_error_handling(EH_THROW, mongo_ce_ConnectionException, &error_handler TSRMLS_CC);

		if (php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_SSLv23_CLIENT, NULL TSRMLS_CC) < 0) {
			zend_restore_error_handling(&error_handler TSRMLS_CC);;
			*error_message = strdup("Cannot setup SSL, is ext/openssl loaded?");
			php_stream_close(stream);
			return NULL;
		}

		crypto_enabled = php_stream_xport_crypto_enable(stream, 1 TSRMLS_CC);
		zend_restore_error_handling(&error_handler TSRMLS_CC);;

		if (crypto_enabled < 0) {
			/* Setting up crypto failed. Thats only OK if we only preferred it */
			if (options->ssl == MONGO_SSL_PREFER) {
				/* FIXME: We can't actually get here because we reject setting
				 * this option to prefer in mcon/parse.c. This is however
				 * probably what we need to do in the future when mongod starts
				 * actually supporting this! :) */
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

	/* Socket timeout behavior uses the same logic as connectTimeoutMS */
	if (options->socketTimeoutMS) {
		struct timeval rtimeout = {0, 0};

		/* Convert negative value to -1 second, which implies no timeout */
		int socketTimeoutMS = options->socketTimeoutMS < 0 ? -1000 : options->socketTimeoutMS;

		rtimeout.tv_sec = socketTimeoutMS / 1000;
		rtimeout.tv_usec = (socketTimeoutMS % 1000) * 1000;
		php_stream_set_option(stream, PHP_STREAM_OPTION_READ_TIMEOUT, 0, &rtimeout);
		mongo_manager_log(MonGlo(manager), MLOG_CON, MLOG_FINE, "Setting stream timeout to %d.%06d", rtimeout.tv_sec, rtimeout.tv_usec);
	}


	/* Avoid a weird leak warning in debug mode when freeing the stream */
#if ZEND_DEBUG
	stream->__exposed = 1;
#endif

	return stream;

}

/* Returns the bytes read on success
 * Returns -31 on unknown failure
 * Returns -80 on timeout
 * Returns -32 when remote server closes the connection
 */
int php_mongo_io_stream_read(mongo_connection *con, mongo_server_options *options, int timeout, void *data, int size, char **error_message)
{
	int num = 1, received = 0, revert_timeout = 0;
	int socketTimeoutMS = options->socketTimeoutMS;
	struct timeval rtimeout = {0, 0};
	TSRMLS_FETCH();

	/* Use default_socket_timeout INI setting if zero */
	if (socketTimeoutMS == 0) {
		socketTimeoutMS = FG(default_socket_timeout) * 1000;
	}

	/* Convert negative values to -1 second, which implies no timeout */
	socketTimeoutMS = socketTimeoutMS < 0 ? -1000 : socketTimeoutMS;
	timeout = timeout < 0 ? -1000 : timeout;

	/* Socket timeout behavior varies based on the following:
	 * - Negative => no timeout (i.e. block indefinitely)
	 * - Zero => not specified (no changes to existing configuration)
	 * - Positive => used specified timeout (revert to previous value later) */
	if (timeout && timeout != socketTimeoutMS) {
		rtimeout.tv_sec = timeout / 1000;
		rtimeout.tv_usec = (timeout % 1000) * 1000;

		revert_timeout = 1; /* We'll want to revert to the old timeout later */
		php_stream_set_option(con->socket, PHP_STREAM_OPTION_READ_TIMEOUT, 0, &rtimeout);
		mongo_manager_log(MonGlo(manager), MLOG_CON, MLOG_FINE, "Setting the stream timeout to %d.%06d", rtimeout.tv_sec, rtimeout.tv_usec);
	} else {
		/* Calculate this now in case we need it for the "timed_out" error message */
		rtimeout.tv_sec = socketTimeoutMS / 1000;
		rtimeout.tv_usec = (socketTimeoutMS % 1000) * 1000;

		mongo_manager_log(MonGlo(manager), MLOG_CON, MLOG_FINE, "No timeout changes for %s", con->hash);
	}

	php_mongo_stream_notify_io(options, MONGO_STREAM_NOTIFY_IO_READ, 0, size TSRMLS_CC);

	/* this can return FAILED if there is just no more data from db */
	while (received < size && num > 0) {
		int len = 4096 < (size - received) ? 4096 : size - received;
		zend_error_handling error_handler;

		zend_replace_error_handling(EH_THROW, mongo_ce_ConnectionException, &error_handler TSRMLS_CC);
		num = php_stream_read(con->socket, (char *) data, len);
		zend_restore_error_handling(&error_handler TSRMLS_CC);;

		if (num < 0) {
			/* Doesn't look like this can happen, php_sockop_read overwrites
			 * the failure from recv() to return 0 */
			*error_message = strdup("Read from socket failed");
			return -31;
		}

		/* It *may* have failed. It also may simply have no data */
		if (num == 0) {
			zval *metadata;

			MAKE_STD_ZVAL(metadata);
			array_init(metadata);
			if (php_stream_populate_meta_data(con->socket, metadata)) {
				zval **tmp;

				if (zend_hash_find(Z_ARRVAL_P(metadata), "timed_out", sizeof("timed_out"), (void**)&tmp) == SUCCESS) {
					convert_to_boolean_ex(tmp);
					if (Z_BVAL_PP(tmp)) {
						*error_message = malloc(256);
						snprintf(*error_message, 256, "Read timed out after reading %d bytes, waited for %d.%06d seconds", num, rtimeout.tv_sec, rtimeout.tv_usec);
						zval_ptr_dtor(&metadata);
						return -80;
					}
				}
				if (zend_hash_find(Z_ARRVAL_P(metadata), "eof", sizeof("eof"), (void**)&tmp) == SUCCESS) {
					convert_to_boolean_ex(tmp);
					if (Z_BVAL_PP(tmp)) {
						*error_message = strdup("Remote server has closed the connection");
						zval_ptr_dtor(&metadata);
						return -32;
					}
				}
			}
			zval_ptr_dtor(&metadata);
		}

		data = (char*)data + num;
		received += num;
	}
	/* PHP may have sent notify-progress of *more then* 'received' in some
	 * cases.
	 * PHP will read 8192 byte chunks at a time, but if we request less data
	 * then that PHP will just buffer the rest, which is fine.  It could
	 * confuse users a little, why their progress update was higher then the
	 * max-bytes-expected though... */
	php_mongo_stream_notify_io(options, MONGO_STREAM_NOTIFY_IO_COMPLETED, received, size TSRMLS_CC);

	/* If the timeout was changed, revert to the previous value now */
	if (revert_timeout) {
		/* If socketTimeoutMS was never specified, revert to default_socket_timeout */
		if (options->socketTimeoutMS == 0) {
			mongo_manager_log(MonGlo(manager), MLOG_CON, MLOG_FINE, "Stream timeout will be reverted to default_socket_timeout (%d)", FG(default_socket_timeout));
		}

		rtimeout.tv_sec = socketTimeoutMS / 1000;
		rtimeout.tv_usec = (socketTimeoutMS % 1000) * 1000;

		php_stream_set_option(con->socket, PHP_STREAM_OPTION_READ_TIMEOUT, 0, &rtimeout);
		mongo_manager_log(MonGlo(manager), MLOG_CON, MLOG_FINE, "Now setting stream timeout back to %d.%06d", rtimeout.tv_sec, rtimeout.tv_usec);
	}

	return received;
}

int php_mongo_io_stream_send(mongo_connection *con, mongo_server_options *options, void *data, int size, char **error_message)
{
	int retval;
	zend_error_handling error_handler;
	TSRMLS_FETCH();

	php_mongo_stream_notify_io(options, MONGO_STREAM_NOTIFY_IO_WRITE, 0, size TSRMLS_CC);

	zend_replace_error_handling(EH_THROW, mongo_ce_ConnectionException, &error_handler TSRMLS_CC);
	retval = php_stream_write(con->socket, (char *) data, size);
	zend_restore_error_handling(&error_handler TSRMLS_CC);;
	if (retval >= size) {
		php_mongo_stream_notify_io(options, MONGO_STREAM_NOTIFY_IO_COMPLETED, size, size TSRMLS_CC);
	}

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

	/* When we fork we need to unregister the parents hash so we don't
	 * accidentally destroy it */
	if (zend_hash_find(&EG(persistent_list), con->hash, strlen(con->hash) + 1, (void*) &le) == SUCCESS) {
		((php_stream *)con->socket)->in_free = 1;
		zend_hash_del(&EG(persistent_list), con->hash, strlen(con->hash) + 1);
		((php_stream *)con->socket)->in_free = 0;
	}
}

#if HAVE_MONGO_SASL
int is_sasl_failure(sasl_conn_t *conn, int result, char **error_message)
{
	if (result < 0) {
		*error_message = malloc(256);
		snprintf(*error_message, 256, "Authentication error: %s", sasl_errstring(result, NULL, NULL));
		return 1;
	}

	return 0;
}

sasl_conn_t *php_mongo_saslstart(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, sasl_conn_t *conn, char **out_payload, int *out_payload_len, int32_t *conversation_id, char **error_message)
{
	const char *raw_payload;
	char encoded_payload[4096];
	unsigned int raw_payload_len, encoded_payload_len;
	int result;
	char *mechanism_list;
	const char *mechanism_selected;
	sasl_interact_t *client_interact=NULL;

	/* Intentionally only send the mechanism we expect to authenticate with, rather then
	 * list of all supported ones. This is because MongoDB doesn't support negotiating */
	switch(server_def->mechanism) {
		case MONGO_AUTH_MECHANISM_SCRAM_SHA1:
			/* cyrus-sasl calls it just "SCRAM" */
			mechanism_list = "SCRAM";
			break;

		case MONGO_AUTH_MECHANISM_GSSAPI:
		default:
			mechanism_list = "GSSAPI";

	}

	result = sasl_client_start(conn, mechanism_list, &client_interact, &raw_payload, &raw_payload_len, &mechanism_selected);
	if (is_sasl_failure(conn, result, error_message)) {
		return NULL;
	}

	if (result != SASL_CONTINUE) {
		*error_message = strdup("Could not negotiate SASL mechanism");
		return NULL;
	}

	result = sasl_encode64(raw_payload, raw_payload_len, encoded_payload, sizeof(encoded_payload), &encoded_payload_len);
	if (is_sasl_failure(conn, result, error_message)) {
		return NULL;
	}

	/* We don't care for whatever was mechanism_selected, we carry on with mechanism_list as that contains the only mechanism we want to use */
	if (!mongo_connection_authenticate_saslstart(manager, con, options, server_def, mechanism_list, encoded_payload, encoded_payload_len + 1, out_payload, out_payload_len, conversation_id, error_message)) {
		return NULL;
	}

	return conn;
}

int php_mongo_saslcontinue(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, sasl_conn_t *conn, char *step_payload, int step_payload_len, int32_t conversation_id, char **error_message) {
	sasl_interact_t *client_interact=NULL;

	/*
	 * Snippet from sasl.h:
	 *  4. client calls sasl_client_step()
	 *  4b. If SASL error, goto 7 or 3
	 *  4c. If SASL_OK, continue or goto 6 if last server response was success
	 */

	do {
		char base_payload[4096], payload[4096];
		unsigned int base_payload_len, payload_len;
		const char *out;
		unsigned int outlen;
		unsigned char done = 0;
		int result;

		step_payload_len--; /* Remove the \0 from the string */
		result = sasl_decode64(step_payload, step_payload_len, base_payload, sizeof(base_payload), &base_payload_len);
		if (is_sasl_failure(conn, result, error_message)) {
			return 0;
		}

		result = sasl_client_step(conn, (const char *)base_payload, base_payload_len, &client_interact, &out, &outlen);
		if (is_sasl_failure(conn, result, error_message)) {
			return 0;
		}

		result = sasl_encode64(out, outlen, payload, sizeof(base_payload), &payload_len);
		if (is_sasl_failure(conn, result, error_message)) {
			return 0;
		}

		if (!mongo_connection_authenticate_saslcontinue(manager, con, options, server_def, conversation_id, payload, payload_len + 1, &step_payload, &step_payload_len, &done, error_message)) {
			return 0;
		}

		if (done) {
			break;
		}
	} while (1);

	return 1;
}

/* Callback function used by SASL when requesting the password */
static int sasl_interact_secret(sasl_conn_t *conn, mongo_server_def *server_def, int id, sasl_secret_t **psecret)
{
	switch (id) {
		case SASL_CB_PASS: {
			char *password;
			int len;

			/* MongoDB uses the legacy MongoDB-CR hash as the SCRAM-SHA-1 password */
			if (server_def->mechanism == MONGO_AUTH_MECHANISM_SCRAM_SHA1) {
				password = mongo_authenticate_hash_user_password(server_def->username, server_def->password);
			} else {
				password = server_def->password;
			}

			len = strlen(password);
			*psecret = malloc(sizeof(sasl_secret_t) + len);
			(*psecret)->len = len;
			memcpy((*psecret)->data, password, (*psecret)->len);

			return SASL_OK;
		}
	}

	return SASL_FAIL;
}

/* Callback function used by SASL when requesting the username/authname */
static int sasl_interact_simple(mongo_server_def *server_def, int id, const char **result, unsigned *len)
{
	switch (id) {
		case SASL_CB_AUTHNAME:
		case SASL_CB_USER:
			*result = server_def->username;
			if (len) {
				*len = server_def->username ? strlen(server_def->username) : 0;
			}
			return SASL_OK;

		case SASL_CB_LANGUAGE: /* NOT SUPPORTED */
			break;
	}

	return SASL_FAIL;
}

int php_mongo_io_authenticate_sasl(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, char **error_message)
{
	int result;
	char *initpayload;
	int initpayload_len;
	sasl_conn_t *conn;
	int32_t conversation_id;
	sasl_callback_t client_interact [] = {
		{ SASL_CB_AUTHNAME, sasl_interact_simple, server_def },
		{ SASL_CB_USER,     sasl_interact_simple, server_def },
		{ SASL_CB_PASS,     sasl_interact_secret, server_def },
		{ SASL_CB_LIST_END, NULL, NULL }
	};

	result = sasl_client_new(options->gssapiServiceName, server_def->host, NULL, NULL, client_interact, 0, &conn);

	if (result != SASL_OK) {
		sasl_dispose(&conn);
		*error_message = strdup("Could not initialize a client exchange (SASL) to MongoDB");
		return 0;
	}

	conn = php_mongo_saslstart(manager, con, options, server_def, conn, &initpayload, &initpayload_len, &conversation_id, error_message);
	if (!conn) {
		/* error message populate by php_mongo_saslstart() */
		return 0;
	}

	if (!php_mongo_saslcontinue(manager, con, options, server_def, conn, initpayload, initpayload_len, conversation_id, error_message)) {
		return 0;
	}

	free(initpayload);
	sasl_dispose(&conn);

	return 1;
}

int php_mongo_io_authenticate_plain(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, char **error_message)
{
	char step_payload[4096];
	char *out, *plain;
	char *mechanism = "PLAIN";
	unsigned int step_payload_len, plain_len;
	int outlen;
	int32_t step_conversation_id;
	int result;

	plain_len = spprintf(&plain, 0, "%c%s%c%s", '\0', server_def->username, '\0', server_def->password);

	result = sasl_encode64(plain, plain_len, step_payload, sizeof(step_payload), &step_payload_len);
	if (result != SASL_OK) {
		*error_message = strdup("SASL authentication: Could not base64 encode payload");
		efree(plain);
		return 0;
	}
	efree(plain);

	if (!mongo_connection_authenticate_saslstart(manager, con, options, server_def, mechanism, step_payload, step_payload_len + 1, &out, &outlen, &step_conversation_id, error_message)) {
		return 0;
	}
	free(out);

	return 1;
}
#endif

int php_mongo_io_stream_authenticate(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, char **error_message)
{
	switch(server_def->mechanism) {
		case MONGO_AUTH_MECHANISM_MONGODB_CR:
		case MONGO_AUTH_MECHANISM_MONGODB_X509:
			/* Use the mcon implementation of MongoDB-CR (current default) and MongoDB-X509 */
			return mongo_connection_authenticate(manager, con, options, server_def, error_message);

#if HAVE_MONGO_SASL
		case MONGO_AUTH_MECHANISM_GSSAPI:
		case MONGO_AUTH_MECHANISM_SCRAM_SHA1:
			return php_mongo_io_authenticate_sasl(manager, con, options, server_def, error_message);

		case MONGO_AUTH_MECHANISM_PLAIN:
			return php_mongo_io_authenticate_plain(manager, con, options, server_def, error_message);

#endif

		default:
#if HAVE_MONGO_SASL
			*error_message = strdup("Unknown authentication mechanism. Only SCRAM-SHA-1, MongoDB-CR, MONGODB-X509, GSSAPI and PLAIN are supported by this build");
#else
			*error_message = strdup("Unknown authentication mechanism. Only MongoDB-CR and MONGODB-X509 are supported by this build");
#endif
	}

	return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
