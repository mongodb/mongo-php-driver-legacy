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
#include "contrib/crypto.h"
#include "api/wire_version.h"

#ifdef PHP_WIN32
# include "config.w32.h"
#else
# include <php_config.h>
#endif

#ifdef HAVE_OPENSSL_EXT
# include "contrib/php-ssl.h"
#endif

#include <php.h>
#include <main/php_streams.h>
#include <main/php_network.h>
#include <ext/standard/file.h>
#include <ext/standard/sha1.h>
#include <ext/standard/base64.h>
#include <ext/standard/php_string.h>

#if PHP_WIN32
# include "win32/winutil.h"
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_MONGO_SASL
#include <sasl/sasl.h>
#include <sasl/saslutil.h>
#endif

#define PHP_MONGO_SCRAM_HASH_1     "SCRAM-SHA-1"
#define PHP_MONGO_SCRAM_SERVER_KEY "Server Key"
#define PHP_MONGO_SCRAM_CLIENT_KEY "Client Key"
#define PHP_MONGO_SCRAM_HASH_SIZE 20

extern zend_class_entry *mongo_ce_ConnectionException;
ZEND_EXTERN_MODULE_GLOBALS(mongo)

#ifdef HAVE_OPENSSL_EXT
# if PHP_VERSION_ID < 50600
int php_mongo_verify_hostname(mongo_server_def *server, X509 *cert TSRMLS_DC)
{
	if (php_mongo_matches_san_list(cert, server->host) == SUCCESS) {
		return SUCCESS;
	}

	if (php_mongo_matches_common_name(cert, server->host TSRMLS_CC) == SUCCESS) {
		return SUCCESS;
	}

	return FAILURE;
}
# endif
#endif

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

		/* Capture the server certificate so we can do further verification */
		if (stream->context) {
			zval capture;
			ZVAL_BOOL(&capture, 1);
			php_stream_context_set_option(stream->context, "ssl", "capture_peer_cert", &capture);
		}

		zend_replace_error_handling(EH_THROW, mongo_ce_ConnectionException, &error_handler TSRMLS_CC);

		/* PHP 5.6.0 until 5.6.7 screwed things a bit, see https://bugs.php.net/bug.php?id=69195 */
#if PHP_VERSION_ID >= 50600 && PHP_VERSION_ID < 50607
		if (php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_ANY_CLIENT, NULL TSRMLS_CC) < 0) {
#else
		if (php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_SSLv23_CLIENT, NULL TSRMLS_CC) < 0) {
#endif
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
		} else if (stream->context) {
#ifdef HAVE_OPENSSL_EXT
			zval **zcert;

			if (php_stream_context_get_option(stream->context, "ssl", "peer_certificate", &zcert) == SUCCESS && Z_TYPE_PP(zcert) == IS_RESOURCE) {
				zval **verify_peer_name, **verify_expiry;
				int resource_type;
				X509 *cert;
				int type;


				zend_list_find(Z_LVAL_PP(zcert), &resource_type);
				cert = (X509 *)zend_fetch_resource(zcert TSRMLS_CC, -1, "OpenSSL X.509", &type, 1, resource_type);

				if (!cert) {
					*error_message = strdup("Couldn't capture remote certificate to validate");
					mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Could not capture certificate of %s:%d", server->host, server->port);
					php_stream_close(stream);
					return NULL;
				}

#if PHP_VERSION_ID < 50600
				/* This option is available since PHP 5.6.0 */
				if (php_stream_context_get_option(stream->context, "ssl", "verify_peer_name", &verify_peer_name) == SUCCESS && zend_is_true(*verify_peer_name)) {
					if (php_mongo_verify_hostname(server, cert TSRMLS_CC) == FAILURE) {
						*error_message = strdup("Cannot verify remote certificate: Hostname doesn't match");
						mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Remote certificate SubjectAltName or CN does not match '%s'", server->host);
						php_stream_close(stream);
						return NULL;
					}
					mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "stream_connect: Valid peer name for %s:%d", server->host, server->port);
				} else {
					mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Not verifying peer name for %s:%d, please use 'verify_peer_name' SSL context option", server->host, server->port);
				}
#endif
				if (php_stream_context_get_option(stream->context, "ssl", "verify_expiry", &verify_expiry) == SUCCESS && zend_is_true(*verify_expiry)) {
					time_t current = time(NULL);
					time_t valid_from  = php_mongo_asn1_time_to_time_t(X509_get_notBefore(cert) TSRMLS_CC);
					time_t valid_until = php_mongo_asn1_time_to_time_t(X509_get_notAfter(cert) TSRMLS_CC);

					if (valid_from > current) {
						*error_message = strdup("Failed expiration check: Certificate is not valid yet");
						mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Certificate is not valid yet on %s:%d", server->host, server->port);
						php_stream_close(stream);
						return NULL;
					}
					if (current > valid_until) {
						*error_message = strdup("Failed expiration check: Certificate has expired");
						mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Certificate has expired on %s:%d", server->host, server->port);
						php_stream_close(stream);
						return NULL;
					}
					mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "stream_connect: Valid issue and expire dates for %s:%d", server->host, server->port);
				} else {
					mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Certificate expiration checks disabled");
				}
			}
#endif /* HAVE_OPENSSL_EXT */
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

		/* Apply socketTimeoutMS in case the timeout was altered by another
		 * MongoClient (the stream may be a persistent connection). From the
		 * perspective of this MongoClient, the timeout is not changing. */
		php_stream_set_option(con->socket, PHP_STREAM_OPTION_READ_TIMEOUT, 0, &rtimeout);
		mongo_manager_log(MonGlo(manager), MLOG_CON, MLOG_FINE, "No timeout changes for %s", con->hash);
	}

	php_mongo_stream_notify_io(con->socket, MONGO_STREAM_NOTIFY_IO_READ, 0, size TSRMLS_CC);

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
	php_mongo_stream_notify_io(con->socket, MONGO_STREAM_NOTIFY_IO_COMPLETED, received, size TSRMLS_CC);

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

	php_mongo_stream_notify_io(con->socket, MONGO_STREAM_NOTIFY_IO_WRITE, 0, size TSRMLS_CC);

	zend_replace_error_handling(EH_THROW, mongo_ce_ConnectionException, &error_handler TSRMLS_CC);
	retval = php_stream_write(con->socket, (char *) data, size);
	zend_restore_error_handling(&error_handler TSRMLS_CC);;
	if (retval >= size) {
		php_mongo_stream_notify_io(con->socket, MONGO_STREAM_NOTIFY_IO_COMPLETED, size, size TSRMLS_CC);
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
static int is_sasl_failure(sasl_conn_t *conn, int result, char **error_message)
{
	if (result < 0) {
		*error_message = malloc(256);
		snprintf(*error_message, 256, "Authentication error: %s", sasl_errstring(result, NULL, NULL));
		return 1;
	}

	return 0;
}

static sasl_conn_t *php_mongo_saslstart(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, sasl_conn_t *conn, char **out_payload, int *out_payload_len, int32_t *conversation_id, char **error_message)
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
		/* NOTE: We don't use cyrus-sasl for SCRAM-SHA-1, but it's left here as it's easier to support multiple SASL mechanisms this way */
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

static int php_mongo_saslcontinue(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, sasl_conn_t *conn, char *step_payload, int step_payload_len, int32_t conversation_id, char **error_message)
{
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

/*
 * client-first-message-bare          = username "," nonce ["," extensions]
 *
 * client-first-message               = gs2-header client-first-message-bare
 * server-first-message               = [reserved-mext ","] nonce "," salt ","
 *                                      iteration-count ["," extensions]
 * client-final-message-without-proof = channel-binding "," nonce ["," extensions]
 *
 * SaltedPassword                    := Hi(Normalize(password), salt, i)
 * ClientKey                         := HMAC(SaltedPassword, "Client Key")
 * StoredKey                         := H(ClientKey)
 * AuthMessage                       := client-first-message-bare + "," +
 *                                          server-first-message + "," +
 *                                          client-final-message-without-proof
 * ClientSignature                   := HMAC(StoredKey, AuthMessage)
 * ClientProof                       := ClientKey XOR ClientSignature
 * ServerKey                         := HMAC(SaltedPassword, "Server Key")
 * ServerSignature                   := HMAC(ServerKey, AuthMessage)
*/

int php_mongo_io_make_client_proof(char *username, char *password, unsigned char *salt_base64, int salt_base64_len, int iterations, char **return_value, int *return_value_len, char *server_first_message, unsigned char *cnonce, char *snonce, unsigned char *server_signature, int *server_signature_len TSRMLS_DC)
{
	unsigned char stored_key[PHP_MONGO_SCRAM_HASH_SIZE], client_proof[PHP_MONGO_SCRAM_HASH_SIZE], client_signature[PHP_MONGO_SCRAM_HASH_SIZE];
	unsigned char salted_password[PHP_MONGO_SCRAM_HASH_SIZE], client_key[PHP_MONGO_SCRAM_HASH_SIZE], server_key[PHP_MONGO_SCRAM_HASH_SIZE];
	unsigned char *salt, *auth_message;
	long salted_password_len;
	int salt_len, client_key_len, server_key_len;
	int auth_message_len, client_signature_len;
	int i;


	salt = php_base64_decode(salt_base64, salt_base64_len, &salt_len);

	/* SaltedPassword  := Hi(Normalize(password), salt, i) */
	php_mongo_hash_pbkdf2_sha1(password, strlen(password), salt, salt_len, iterations, salted_password, &salted_password_len TSRMLS_CC);
	efree(salt);

	/* ClientKey       := HMAC(SaltedPassword, "Client Key") */
	php_mongo_hmac((unsigned char *)PHP_MONGO_SCRAM_CLIENT_KEY, strlen((char *)PHP_MONGO_SCRAM_CLIENT_KEY), (char *)salted_password, salted_password_len, client_key, &client_key_len);

	/* StoredKey       := H(ClientKey) */
	php_mongo_sha1(client_key, client_key_len, stored_key);

	/* AuthMessage     := client-first-message-bare + "," +
	 *                    server-first-message + "," +
	 *                    client-final-message-without-proof
	 */
	auth_message_len  = spprintf((char **)&auth_message, 0, "n=%s,r=%s,%s,c=biws,%s", username, cnonce, server_first_message, snonce);

	/* ClientSignature := HMAC(StoredKey, AuthMessage) */
	php_mongo_hmac(auth_message, auth_message_len, (char *)stored_key, PHP_MONGO_SCRAM_HASH_SIZE, (unsigned char *)client_signature, &client_signature_len);

	/* ClientProof := ClientKey XOR ClientSignature */
	for (i = 0; i < PHP_MONGO_SCRAM_HASH_SIZE; i++) {
		client_proof[i] = client_key[i] ^ client_signature[i];
	}

	/* ServerKey       := HMAC(SaltedPassword, "Server Key") */
	php_mongo_hmac((unsigned char *)PHP_MONGO_SCRAM_SERVER_KEY, strlen((char *)PHP_MONGO_SCRAM_SERVER_KEY), (char *)salted_password, salted_password_len, (unsigned char *)server_key, &server_key_len);

	/* ServerSignature := HMAC(ServerKey, AuthMessage) */
	php_mongo_hmac(auth_message, auth_message_len, (char *)server_key, PHP_MONGO_SCRAM_HASH_SIZE, server_signature, server_signature_len);
	efree(auth_message);

	*return_value = (char *)php_base64_encode(client_proof, PHP_MONGO_SCRAM_HASH_SIZE, return_value_len);

	return 1;
}

/**
 * Authenticates a connection using SCRAM-SHA-1
 *
 * Returns:
 * 0: when it didn't work - with the error_message set.
 * 1: when it worked
 * 2: when no need to authenticate (i.e. no credentials provided)
 */
int mongo_connection_authenticate_mongodb_scram_sha1(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, char **error_message)
{
	char *client_first_message, *client_first_message_base64;
	char *client_final_message, *client_final_message_base64;
	int client_first_message_len, client_first_message_base64_len;
	int client_final_message_len, client_final_message_base64_len;

	char *server_first_message, *server_first_message_base64, *server_first_message_dup;
	char *server_final_message, *server_final_message_base64;
	int server_first_message_len, server_first_message_base64_len;
	int server_final_message_len, server_final_message_base64_len;

	char *rnonce, *password, *tok, *proof = NULL;
	char *iterationsstr, *salt_base64;
	int rskip, iterations, proof_len;
	unsigned char cnonce[41];
	int32_t step_conversation_id;
	unsigned char done = 0;
	char *tmp, *username;
	int username_len;
	unsigned char server_signature[PHP_MONGO_SCRAM_HASH_SIZE];
	unsigned char *server_signature_base64;
	int server_signature_len, server_signature_base64_len;
	TSRMLS_FETCH();

	if (!server_def->db || !server_def->username || !server_def->password) {
		return 2;
	}

	/*
	 * The characters ',' or '=' in usernames are sent as '=2C' and
	 * '=3D' respectively.  If the server receives a username that
	 * contains '=' not followed by either '2C' or '3D', then the
	 * server MUST fail the authentication
	 */
	tmp = php_str_to_str(server_def->username, strlen(server_def->username), "=", 1, "=3D", 3, &username_len);
	username = php_str_to_str(tmp, strlen(tmp), ",", 1, "=2C", 3, &username_len);
	efree(tmp);

	php_mongo_io_make_nonce((char *)cnonce TSRMLS_CC);

	/*
	 * client-first-message      = gs2-header client-first-message-bare
	 * client-first-message-bare = username "," nonce
	 * username                  = "n=" saslname
	 * nonce                     = "r=" c-nonce [s-nonce]
	 *                             ;; Second part provided by server.
	 * c-nonce                   = printable (client generated nonce)
	 * s-nonce                   = printable (server appending nonce)
	 * gs2-header                = gs2-cbind-flag "," [ authzid ] ","
	 *
	 * We don't support GS2, so that becomes "n,,"
	 * example: n,,n=user,r=fyko+d2lbbFgONRv9qkxdawL
	 */
	client_first_message_len = spprintf(&client_first_message, 0, "n,,n=%s,r=%s", username, cnonce);
	client_first_message_base64 = (char *)php_base64_encode((unsigned char *)client_first_message, client_first_message_len, &client_first_message_base64_len);

	if (!mongo_connection_authenticate_saslstart(manager, con, options, server_def, PHP_MONGO_SCRAM_HASH_1, client_first_message_base64, client_first_message_base64_len+1, &server_first_message_base64, &server_first_message_base64_len, &step_conversation_id, error_message)) {
		efree(client_first_message);
		efree(client_first_message_base64);
		efree(username);
		/* starting sasl failed, bail out, we do not need to send an error message, as
		 * mongo_connection_authenticate_saslstart already does so when returning a 0
		 * error value. */
		return 0;
	}
	efree(client_first_message_base64);


	/*
	 * server-first-message = nonce "," salt "," iteration-count
	 * nonce                     = "r=" c-nonce [s-nonce]
	 *                             ;; Second part provided by server.
	 * c-nonce                   = printable (client generated nonce)
	 * s-nonce                   = printable (server appending nonce)
	 * salt                 = "s=" base64
	 * iteration-count      = "i=" posit-number
	 *
	 * example: r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,s=QSXCR+Q6sek8bf92,i=10000
	 */
	server_first_message = (char *)php_base64_decode((unsigned char *)server_first_message_base64, server_first_message_base64_len, &server_first_message_len);
	free(server_first_message_base64);
	server_first_message_dup = estrdup(server_first_message);

	/* the r= from the client_first_message appended with more chars from the server */
	rskip = username_len+6; /* n,,n= and the coma before r */

	rnonce = php_strtok_r(server_first_message_dup, ",", &tok);
	salt_base64 = php_strtok_r(NULL, ",", &tok)+2;
	iterationsstr = php_strtok_r(NULL, ",", &tok)+2;
	if (rnonce == NULL || salt_base64 == NULL || iterationsstr == NULL) {
		efree(server_first_message);
		efree(server_first_message_dup);
		efree(client_first_message);
		/* the server didn't return our hash, bail out */
		*error_message = strdup("Server return payload in wrong format");
		efree(username);
		return 0;
	}

	if (strncmp(rnonce, client_first_message+rskip, (PHP_MONGO_SCRAM_HASH_SIZE*2)+1-rskip) != 0) {
		efree(server_first_message);
		efree(server_first_message_dup);
		efree(client_first_message);
		/* the server didn't return our hash, bail out */
		*error_message = strdup("Server return invalid hash");
		efree(username);
		return 0;
	}
	efree(client_first_message);

	iterations = strtoll(iterationsstr, NULL, 10);
	/* MongoDB uses the legacy MongoDB-CR hash as the SCRAM-SHA-1 password */
	password = mongo_authenticate_hash_user_password(username, server_def->password);
	php_mongo_io_make_client_proof(username, password, (unsigned char*)salt_base64, strlen(salt_base64), iterations, &proof, &proof_len, server_first_message, cnonce, rnonce, server_signature, &server_signature_len TSRMLS_CC);
	efree(username);
	efree(server_first_message);
	free(password);

	/*
	 * c: This REQUIRED attribute specifies the base64-encoded GS2 header
	 *    and channel binding data.  It is sent by the client in its second
	 *    authentication message.  The attribute data consist of:...
	 *    We don't support GS2 nor channel binding, so set this to:
	 *        biws == base64_encode("n,,")
	 *
	 * r: This attribute specifies a sequence of random printable ASCII
	 *    characters excluding ',' (which forms the nonce used as input to
	 *    the hash function).  No quoting is applied to this string.  As
	 *    described earlier, the client supplies an initial value in its
	 *    first message, and the server augments that value with its own
	 *    nonce in its first response.  It is important that this value be
	 *    different for each authentication (see [RFC4086] for more details
	 *    on how to achieve this).  The client MUST verify that the initial
	 *    part of the nonce used in subsequent messages is the same as the
	 *    nonce it initially specified.  The server MUST verify that the
	 *    nonce sent by the client in the second message is the same as the
	 *    one sent by the server in its first message.
	 *
	 * p: This attribute specifies a base64-encoded ClientProof.  The
	 *    client computes this value as described in the overview and sends
	 *    it to the server.
	 */
	/*
	 * client-final-message =
	 *                      client-final-message-without-proof "," proof
	 *
	 * client-final-message-without-proof =
	 *                      channel-binding "," nonce ["," extensions]
	 *
	 * proof           = "p=" base64
	 *
	 * nonce           = "r=" c-nonce [s-nonce]
	 *                   ;; Second part provided by server.
	 *
	 * channel-binding = "c=" base64
	 *                   ;; base64 encoding of cbind-input.
	 *
	 * cbind-input     = (we don't support these things, see 'c:' explaination above)
	 *
	 * example: c=biws,r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,p=v0X8v3Bz2T0CJGbJQyF0X+HI4Ts=
	 */
	client_final_message_len = spprintf(&client_final_message, 0, "c=biws,%s,p=%s", rnonce, proof);
	efree(proof);
	efree(server_first_message_dup);
	/* base64 for the server (payload), or BSON Binary encode.. simpler to base64 */
	client_final_message_base64 = (char *)php_base64_encode((unsigned char*)client_final_message, client_final_message_len, &client_final_message_base64_len);

	if (!mongo_connection_authenticate_saslcontinue(manager, con, options, server_def, step_conversation_id, client_final_message_base64, client_final_message_base64_len+1, &server_final_message_base64, &server_final_message_base64_len, &done, error_message)) {
		efree(client_final_message);
		efree(client_final_message_base64);
		return 0;
	}
	efree(client_final_message);
	efree(client_final_message_base64);

	/* Verify the server signature */
	server_final_message = (char *)php_base64_decode((unsigned char*)server_final_message_base64, server_final_message_base64_len, &server_final_message_len);
	server_signature_base64 = php_base64_encode((unsigned char*)server_signature, server_signature_len, &server_signature_base64_len);
	if (strncmp(server_final_message+2, (char *)server_signature_base64, server_signature_base64_len) != 0) {
		efree(server_final_message);
		*error_message = strdup("Server returned wrong ServerSignature");
		return 0;
	}
	efree(server_final_message);
	efree(server_signature_base64);
	free(server_final_message_base64);

	/* Extra roundtrip to let the server know we trust her */
	if (!mongo_connection_authenticate_saslcontinue(manager, con, options, server_def, step_conversation_id, "", 1, &server_final_message_base64, &server_final_message_base64_len, &done, error_message)) {
		free(server_final_message_base64);
		return 0;
	}
	free(server_final_message_base64);

	return 1;
}

int php_mongo_io_stream_authenticate(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, char **error_message)
{
	switch(server_def->mechanism) {
		case MONGO_AUTH_MECHANISM_MONGODB_DEFAULT:
			if (php_mongo_api_connection_supports_feature(con, PHP_MONGO_API_RELEASE_2_8)) {
				return mongo_connection_authenticate_mongodb_scram_sha1(manager, con, options, server_def, error_message);
			}
			return mongo_connection_authenticate(manager, con, options, server_def, error_message);

		case MONGO_AUTH_MECHANISM_MONGODB_CR:
		case MONGO_AUTH_MECHANISM_MONGODB_X509:
			/* Use the mcon implementation of MongoDB-CR and MongoDB-X509 */
			return mongo_connection_authenticate(manager, con, options, server_def, error_message);

		case MONGO_AUTH_MECHANISM_SCRAM_SHA1:
			return mongo_connection_authenticate_mongodb_scram_sha1(manager, con, options, server_def, error_message);
			break;

#if HAVE_MONGO_SASL
		case MONGO_AUTH_MECHANISM_GSSAPI:
			return php_mongo_io_authenticate_sasl(manager, con, options, server_def, error_message);

		case MONGO_AUTH_MECHANISM_PLAIN:
			return php_mongo_io_authenticate_plain(manager, con, options, server_def, error_message);

#endif

		default:
#if HAVE_MONGO_SASL
			*error_message = strdup("Unknown authentication mechanism. Only SCRAM-SHA-1, MongoDB-CR, MONGODB-X509, GSSAPI and PLAIN are supported by this build");
#else
			*error_message = strdup("Unknown authentication mechanism. Only SCRAM-SHA-1, MongoDB-CR and MONGODB-X509 are supported by this build");
#endif
	}

	return 0;
}

void php_mongo_io_make_nonce(char *sha1_str TSRMLS_DC) /* {{{ */
{
	unsigned char digest[20];
	PHP_SHA1_CTX sha1_context;
	size_t to_read = 32;
	unsigned char rbuf[64];

#ifdef PHP_WIN32
	PHP_SHA1Init(&sha1_context);
	if (php_win32_get_random_bytes(rbuf, to_read) == SUCCESS){
		PHP_SHA1Update(&sha1_context, rbuf, to_read);
	}
#else
	int fd;

	PHP_SHA1Init(&sha1_context);
	fd = VCWD_OPEN("/dev/urandom", O_RDONLY);
	if (fd >= 0) {
		int n;

		while (to_read > 0) {
			n = read(fd, rbuf, to_read);
			if (n <= 0) break;

			PHP_SHA1Update(&sha1_context, rbuf, n);
			to_read -= n;
		}
		close(fd);
	}
#endif

	PHP_SHA1Final(digest, &sha1_context);
	make_sha1_digest(sha1_str, digest);
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
