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
#include <php.h>

#include <zend_exceptions.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "php_mongo.h"

#include "mongoclient.h"
#include "mongo.h"
#include "cursor_shared.h"
#include "cursor.h"
#include "command_cursor.h"
#include "io_stream.h"
#include "log_stream.h"

#include "exceptions/exception.h"
#include "exceptions/connection_exception.h"
#include "exceptions/cursor_exception.h"
#include "exceptions/cursor_timeout_exception.h"
#include "exceptions/duplicate_key_exception.h"
#include "exceptions/execution_timeout_exception.h"
#include "exceptions/gridfs_exception.h"
#include "exceptions/protocol_exception.h"
#include "exceptions/result_exception.h"
#include "exceptions/write_concern_exception.h"

#include "types/id.h"

#include "util/log.h"
#include "util/pool.h"

#include "mcon/manager.h"
#include "mcon/utils.h"
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "api/wire_version.h"

#if HAVE_MONGO_SASL
#include <sasl/sasl.h>
#endif

extern zend_object_handlers mongo_default_handlers, mongo_id_handlers;
zend_object_handlers mongo_type_object_handlers;

/** Classes */
extern zend_class_entry *mongo_ce_Exception;
extern zend_class_entry *mongo_ce_ConnectionException;
extern zend_class_entry *mongo_ce_CursorException;
extern zend_class_entry *mongo_ce_CursorTimeoutException;
extern zend_class_entry *mongo_ce_DuplicateKeyException;
extern zend_class_entry *mongo_ce_GridFSException;
extern zend_class_entry *mongo_ce_ProtocolException;
extern zend_class_entry *mongo_ce_ResultException;
extern zend_class_entry *mongo_ce_WriteConcernException;

zend_class_entry *mongo_ce_MaxKey, *mongo_ce_MinKey;

static void mongo_init_MongoExceptions(TSRMLS_D);

ZEND_DECLARE_MODULE_GLOBALS(mongo)

static PHP_GINIT_FUNCTION(mongo);
static PHP_GSHUTDOWN_FUNCTION(mongo);

zend_function_entry mongo_functions[] = {
	PHP_FE(bson_encode, NULL)
	PHP_FE(bson_decode, NULL)
	PHP_FE_END
};

/* {{{ mongo_module_entry
 */
static const zend_module_dep mongo_deps[] = {
	ZEND_MOD_OPTIONAL("openssl")
#if PHP_VERSION_ID >= 50307
	ZEND_MOD_END
#else /* pre-5.3.7 */
	{ NULL, NULL, NULL, 0 }
#endif
};
zend_module_entry mongo_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	mongo_deps,
	PHP_MONGO_EXTNAME,
	mongo_functions,
	PHP_MINIT(mongo),
	PHP_MSHUTDOWN(mongo),
	PHP_RINIT(mongo),
	NULL,
	PHP_MINFO(mongo),
	PHP_MONGO_VERSION,
	PHP_MODULE_GLOBALS(mongo),
	PHP_GINIT(mongo),
	PHP_GSHUTDOWN(mongo),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_MONGO
ZEND_GET_MODULE(mongo)
#endif

static PHP_INI_MH(OnUpdatePingInterval)
{
	long converted_val;

	if (new_value && is_numeric_string(new_value, new_value_length, &converted_val, NULL, 0) == IS_LONG && converted_val > 0) {
		MonGlo(manager)->ping_interval = converted_val;
		return SUCCESS;
	}

	return FAILURE;
}

static PHP_INI_MH(OnUpdateIsMasterInterval)
{
	long converted_val;

	if (new_value && is_numeric_string(new_value, new_value_length, &converted_val, NULL, 0) == IS_LONG && converted_val > 0) {
		MonGlo(manager)->ismaster_interval = converted_val;
		return SUCCESS;
	}

	return FAILURE;
}

#if SIZEOF_LONG == 4
static PHP_INI_MH(OnUpdateNativeLong)
{
	long converted_val;

	if (new_value && is_numeric_string(new_value, new_value_length, &converted_val, NULL, 0) == IS_LONG) {
		if (converted_val != 0) {
			php_error_docref(NULL TSRMLS_CC, E_CORE_ERROR, "To prevent data corruption, you are not allowed to turn on the mongo.native_long setting on 32-bit platforms");
		}
		return SUCCESS;
	}

	return FAILURE;
}
#endif

/* {{{ PHP_INI */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("mongo.default_host", "localhost", PHP_INI_ALL, OnUpdateString, default_host, zend_mongo_globals, mongo_globals)
	STD_PHP_INI_ENTRY("mongo.default_port", "27017", PHP_INI_ALL, OnUpdateLong, default_port, zend_mongo_globals, mongo_globals)
	STD_PHP_INI_ENTRY("mongo.chunk_size", DEFAULT_CHUNK_SIZE_S, PHP_INI_ALL, OnUpdateLong, chunk_size, zend_mongo_globals, mongo_globals)
	STD_PHP_INI_ENTRY("mongo.cmd", "$", PHP_INI_ALL, OnUpdateStringUnempty, cmd_char, zend_mongo_globals, mongo_globals)
#if SIZEOF_LONG == 4
	STD_PHP_INI_ENTRY("mongo.native_long", "0", PHP_INI_ALL, OnUpdateNativeLong, native_long, zend_mongo_globals, mongo_globals)
#else
	STD_PHP_INI_ENTRY("mongo.native_long", "1", PHP_INI_ALL, OnUpdateLong, native_long, zend_mongo_globals, mongo_globals)
#endif
	STD_PHP_INI_ENTRY("mongo.long_as_object", "0", PHP_INI_ALL, OnUpdateLong, long_as_object, zend_mongo_globals, mongo_globals)
	STD_PHP_INI_ENTRY("mongo.allow_empty_keys", "0", PHP_INI_ALL, OnUpdateLong, allow_empty_keys, zend_mongo_globals, mongo_globals)

	PHP_INI_ENTRY("mongo.ping_interval", MONGO_MANAGER_DEFAULT_PING_INTERVAL_S, PHP_INI_ALL, OnUpdatePingInterval)
	PHP_INI_ENTRY("mongo.is_master_interval", MONGO_MANAGER_DEFAULT_MASTER_INTERVAL_S, PHP_INI_ALL, OnUpdateIsMasterInterval)
PHP_INI_END()
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mongo)
{
	zend_class_entry max_key, min_key;

	REGISTER_INI_ENTRIES();

	mongo_init_MongoClient(TSRMLS_C);
	mongo_init_Mongo(TSRMLS_C);
	mongo_init_MongoDB(TSRMLS_C);
	mongo_init_MongoCollection(TSRMLS_C);
	mongo_init_MongoCursorInterface(TSRMLS_C);
	mongo_init_MongoCursor(TSRMLS_C);
	mongo_init_MongoCommandCursor(TSRMLS_C);

	mongo_init_MongoGridFS(TSRMLS_C);
	mongo_init_MongoGridFSFile(TSRMLS_C);
	mongo_init_MongoGridFSCursor(TSRMLS_C);

	mongo_init_MongoWriteBatch(TSRMLS_C);
	mongo_init_MongoInsertBatch(TSRMLS_C);
	mongo_init_MongoUpdateBatch(TSRMLS_C);
	mongo_init_MongoDeleteBatch(TSRMLS_C);

	mongo_init_MongoId(TSRMLS_C);
	mongo_init_MongoCode(TSRMLS_C);
	mongo_init_MongoRegex(TSRMLS_C);
	mongo_init_MongoDate(TSRMLS_C);
	mongo_init_MongoBinData(TSRMLS_C);
	mongo_init_MongoDBRef(TSRMLS_C);

	mongo_init_MongoExceptions(TSRMLS_C);

	mongo_init_MongoTimestamp(TSRMLS_C);
	mongo_init_MongoInt32(TSRMLS_C);
	mongo_init_MongoInt64(TSRMLS_C);

	mongo_init_MongoLog(TSRMLS_C);

	/* Deprecated, but we will keep it for now */
	mongo_init_MongoPool(TSRMLS_C);

	/* MongoMaxKey and MongoMinKey are completely non-interactive: they have no
	 * method, fields, or constants.  */
	INIT_CLASS_ENTRY(max_key, "MongoMaxKey", NULL);
	mongo_ce_MaxKey = zend_register_internal_class(&max_key TSRMLS_CC);
	INIT_CLASS_ENTRY(min_key, "MongoMinKey", NULL);
	mongo_ce_MinKey = zend_register_internal_class(&min_key TSRMLS_CC);

	/* Make mongo objects uncloneable */
	memcpy(&mongo_default_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mongo_default_handlers.clone_obj = NULL;
	mongo_default_handlers.read_property = mongo_read_property;
	mongo_default_handlers.write_property = mongo_write_property;

	/* Mongo type objects */
	memcpy(&mongo_type_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mongo_type_object_handlers.write_property = mongo_write_property;

	/* Add compare_objects for MongoId */
	memcpy(&mongo_id_handlers, &mongo_default_handlers, sizeof(zend_object_handlers));
	mongo_id_handlers.compare_objects = php_mongo_compare_ids;
	mongo_default_handlers.write_property = mongo_write_property;

	/* Start random number generator */
	srand(time(0));

#if HAVE_MONGO_SASL
	/* We need to bootstrap cyrus-sasl once per process */
	if (sasl_client_init(NULL) != SASL_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not initialize SASL library");
		return FAILURE;
	}
#endif

	REGISTER_LONG_CONSTANT("MONGO_STREAMS", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_STREAMS", 1, CONST_PERSISTENT);

#if HAVE_OPENSSL_EXT
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_SSL", 1, CONST_PERSISTENT);
#else
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_SSL", 0, CONST_PERSISTENT);
#endif

	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_AUTH_MECHANISM_MONGODB_CR", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_AUTH_MECHANISM_MONGODB_X509", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_AUTH_MECHANISM_SCRAM_SHA1", 1, CONST_PERSISTENT);
#if HAVE_MONGO_SASL
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_AUTH_MECHANISM_GSSAPI", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_AUTH_MECHANISM_PLAIN", 1, CONST_PERSISTENT);
#else
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_AUTH_MECHANISM_GSSAPI", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_SUPPORTS_AUTH_MECHANISM_PLAIN", 0, CONST_PERSISTENT);
#endif

	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_TYPE_IO_INIT", MONGO_STREAM_NOTIFY_TYPE_IO_INIT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_TYPE_LOG", MONGO_STREAM_NOTIFY_TYPE_LOG, CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_IO_READ", MONGO_STREAM_NOTIFY_IO_READ, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_IO_WRITE", MONGO_STREAM_NOTIFY_IO_WRITE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_IO_PROGRESS", MONGO_STREAM_NOTIFY_IO_PROGRESS, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_IO_COMPLETED", MONGO_STREAM_NOTIFY_IO_COMPLETED, CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_INSERT", MONGO_STREAM_NOTIFY_LOG_INSERT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_QUERY", MONGO_STREAM_NOTIFY_LOG_QUERY, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_UPDATE", MONGO_STREAM_NOTIFY_LOG_UPDATE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_DELETE", MONGO_STREAM_NOTIFY_LOG_DELETE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_GETMORE", MONGO_STREAM_NOTIFY_LOG_GETMORE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_KILLCURSOR", MONGO_STREAM_NOTIFY_LOG_KILLCURSOR, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_BATCHINSERT", MONGO_STREAM_NOTIFY_LOG_BATCHINSERT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_RESPONSE_HEADER", MONGO_STREAM_NOTIFY_LOG_RESPONSE_HEADER, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_WRITE_REPLY", MONGO_STREAM_NOTIFY_LOG_WRITE_REPLY, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_CMD_INSERT", MONGO_STREAM_NOTIFY_LOG_CMD_INSERT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_CMD_UPDATE", MONGO_STREAM_NOTIFY_LOG_CMD_UPDATE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_CMD_DELETE", MONGO_STREAM_NOTIFY_LOG_CMD_DELETE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MONGO_STREAM_NOTIFY_LOG_WRITE_BATCH", MONGO_STREAM_NOTIFY_LOG_WRITE_BATCH, CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mongo)
{
	/* On windows, the max length is 256. Linux doesn't have a limit, but it
	 * will fill in the first 256 chars of hostname even if the actual
	 * hostname is longer. If you can't get a unique character in the first
	 * 256 chars of your hostname, you're doing it wrong. */
	int len;
	char *hostname, host_start[256];
	register ulong hash;

	mongo_globals->default_host = "localhost";
	mongo_globals->default_port = 27017;
	mongo_globals->request_id = 3;
	mongo_globals->chunk_size = DEFAULT_CHUNK_SIZE;
	mongo_globals->cmd_char = "$";

	mongo_globals->errmsg = 0;

	/* from the gnu manual:
	 *     gethostname stores the beginning of the host name in name even if the
	 *     host name won't entirely fit. For some purposes, a truncated host name
	 *     is good enough. If it is, you can ignore the error code.
	 * So we'll ignore the error code.
	 */
	gethostname(host_start, sizeof(host_start));
	host_start[sizeof(host_start) - 1] = '\0';
	hostname = host_start;
	len = strlen(hostname);

	hash = 5381;

	/* from zend_hash.h */
	/* variant with the hash unrolled eight times */
	for (; len >= 8; len -= 8) {
		hash = ((hash << 5) + hash) + *hostname++;
		hash = ((hash << 5) + hash) + *hostname++;
		hash = ((hash << 5) + hash) + *hostname++;
		hash = ((hash << 5) + hash) + *hostname++;
		hash = ((hash << 5) + hash) + *hostname++;
		hash = ((hash << 5) + hash) + *hostname++;
		hash = ((hash << 5) + hash) + *hostname++;
		hash = ((hash << 5) + hash) + *hostname++;
	}

	switch (len) {
		case 7: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
		case 6: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
		case 5: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
		case 4: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
		case 3: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
		case 2: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
		case 1: hash = ((hash << 5) + hash) + *hostname++; break;
		case 0: break;
	}

	mongo_globals->machine = hash;

	mongo_globals->ts_inc = 0;
	mongo_globals->inc = rand() & 0xFFFFFF;

	mongo_globals->log_callback_info = empty_fcall_info;
	mongo_globals->log_callback_info_cache = empty_fcall_info_cache;

	mongo_globals->manager = mongo_init();
	TSRMLS_SET_CTX(mongo_globals->manager->log_context);
	mongo_globals->manager->log_function = php_mcon_log_wrapper;

	mongo_globals->manager->connect               = php_mongo_io_stream_connect;
	mongo_globals->manager->recv_header           = php_mongo_io_stream_read;
	mongo_globals->manager->recv_data             = php_mongo_io_stream_read;
	mongo_globals->manager->send                  = php_mongo_io_stream_send;
	mongo_globals->manager->close                 = php_mongo_io_stream_close;
	mongo_globals->manager->forget                = php_mongo_io_stream_forget;
	mongo_globals->manager->authenticate          = php_mongo_io_stream_authenticate;
	mongo_globals->manager->supports_wire_version = php_mongo_api_supports_wire_version;
}
/* }}} */

PHP_GSHUTDOWN_FUNCTION(mongo)
{
	if (mongo_globals->manager) {
		mongo_deinit(mongo_globals->manager);
	}
}

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(mongo)
{
	UNREGISTER_INI_ENTRIES();

#if HAVE_MONGO_SASL
	sasl_done();
#endif

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(mongo)
{
	MonGlo(log_level) = 0;
	MonGlo(log_module) = 0;

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mongo)
{
	php_info_print_table_start();

	php_info_print_table_header(2, "MongoDB Support", "enabled");
	php_info_print_table_row(2, "Version", PHP_MONGO_VERSION);
	php_info_print_table_row(2, "Streams Support", "enabled");

#if HAVE_OPENSSL_EXT
	php_info_print_table_row(2, "SSL Support", "enabled");
#endif

	php_info_print_table_colspan_header(2, "Supported Authentication Mechanisms");
	php_info_print_table_row(2, "MONGODB-CR", "enabled");
	php_info_print_table_row(2, "SCRAM-SHA-1", "enabled");

#if HAVE_OPENSSL_EXT
	php_info_print_table_row(2, "MONGODB-X509", "enabled");
#else
	php_info_print_table_row(2, "MONGODB-X509", "disabled");
#endif

#if HAVE_MONGO_SASL
	php_info_print_table_row(2, "GSSAPI (Kerberos)", "enabled");
	php_info_print_table_row(2, "PLAIN", "enabled");
#else
	php_info_print_table_row(2, "GSSAPI (Kerberos)", "disabled");
	php_info_print_table_row(2, "PLAIN", "disabled");
#endif

	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

static void mongo_init_MongoExceptions(TSRMLS_D)
{
	mongo_init_MongoException(TSRMLS_C);
	mongo_init_MongoConnectionException(TSRMLS_C);
	mongo_init_MongoCursorException(TSRMLS_C);
	mongo_init_MongoCursorTimeoutException(TSRMLS_C);
	mongo_init_MongoGridFSException(TSRMLS_C);
	mongo_init_MongoResultException(TSRMLS_C);
	mongo_init_MongoWriteConcernException(TSRMLS_C);
	mongo_init_MongoDuplicateKeyException(TSRMLS_C);
	mongo_init_MongoExecutionTimeoutException(TSRMLS_C);
	mongo_init_MongoProtocolException(TSRMLS_C);
}

/* {{{ Creating & freeing Mongo type objects */
void php_mongo_type_object_free(void *object TSRMLS_DC)
{
	mongo_type_object *mto = (mongo_type_object*)object;

	zend_object_std_dtor(&mto->std TSRMLS_CC);

	efree(mto);
}

zend_object_value php_mongo_type_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	PHP_MONGO_OBJ_NEW(mongo_type_object);
}
/* }}} */

/* Shared helper functions */
static mongo_read_preference_tagset *get_tagset_from_array(int tagset_id, zval *ztagset TSRMLS_DC)
{
	HashTable  *tagset = HASH_OF(ztagset);
	zval      **tag;
	int         item_count = 1, fail = 0;
	mongo_read_preference_tagset *tmp_ts = calloc(1, sizeof(mongo_read_preference_tagset));

	zend_hash_internal_pointer_reset(tagset);
	while (zend_hash_get_current_data(tagset, (void **)&tag) == SUCCESS) {
		if (Z_TYPE_PP(tag) != IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Tag %d in tagset %d needs to contain a string", item_count, tagset_id);
			fail = 1;
		} else {
			char *key;
			uint key_len;
			ulong num_key;

			switch (zend_hash_get_current_key_ex(tagset, &key, &key_len, &num_key, 0, NULL)) {
				case HASH_KEY_IS_LONG:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Tag %d in tagset %d has no string key", item_count, tagset_id);
					fail = 1;
					break;
				case HASH_KEY_IS_STRING:
					mongo_read_preference_add_tag(tmp_ts, key, Z_STRVAL_PP(tag));
					break;
			}

		}
		item_count++;
		zend_hash_move_forward(tagset);
	}
	if (fail) {
		mongo_read_preference_tagset_dtor(tmp_ts);
		return NULL;
	}
	return tmp_ts;
}

/* Returns an array of key=>value pairs, per tagset, from a
 * mongo_read_preference.  This maps to the structure on how mongos expects
 * them */
zval *php_mongo_make_tagsets(mongo_read_preference *rp)
{
	zval *tagsets, *tagset;
	int   i, j;

	if (!rp->tagset_count) {
		return NULL;
	}

	MAKE_STD_ZVAL(tagsets);
	array_init(tagsets);

	for (i = 0; i < rp->tagset_count; i++) {
		MAKE_STD_ZVAL(tagset);
		array_init(tagset);

		for (j = 0; j < rp->tagsets[i]->tag_count; j++) {
			char *name, *colon;
			char *tag = rp->tagsets[i]->tags[j];

			/* Split the "dc:ny" into ["dc" => "ny"] */
			colon = strchr(tag, ':');
			name = zend_strndup(tag, colon - tag);

			add_assoc_string(tagset, name, colon + 1, 1);
		}

		add_next_index_zval(tagsets, tagset);
	}

	return tagsets;
}

void php_mongo_add_tagsets(zval *return_value, mongo_read_preference *rp)
{
	zval *tagsets = php_mongo_make_tagsets(rp);

	if (!tagsets) {
		return;
	}

	add_assoc_zval_ex(return_value, "tagsets", sizeof("tagsets"), tagsets);
}

/* Applies an array of tagsets to the read preference. This function clears the
 * read preference before adding tagsets. If an error is encountered adding a
 * tagset, the read preference will again be cleared to avoid being left in an
 * inconsistent state. */
static int php_mongo_use_tagsets(mongo_read_preference *rp, HashTable *tagsets TSRMLS_DC)
{
	zval **tagset;
	int    item_count = 1;
	mongo_read_preference_tagset *tagset_tmp;

	/* Clear existing tagsets */
	mongo_read_preference_dtor(rp);

	zend_hash_internal_pointer_reset(tagsets);
	while (zend_hash_get_current_data(tagsets, (void **)&tagset) == SUCCESS) {
		if (Z_TYPE_PP(tagset) != IS_ARRAY) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Tagset %d needs to contain an array of 0 or more tags", item_count);
			/* Clear any added tagsets to avoid an inconsistent state */
			mongo_read_preference_dtor(rp);
			return 0;
		} else {
			tagset_tmp = get_tagset_from_array(item_count, *tagset TSRMLS_CC);
			if (tagset_tmp) {
				mongo_read_preference_add_tagset(rp, tagset_tmp);
			} else {
				/* Clear any added tagsets to avoid an inconsistent state */
				mongo_read_preference_dtor(rp);
				return 0;
			}
		}
		item_count++;
		zend_hash_move_forward(tagsets);
	}
	return 1;
}

/* Sets read preference mode and tagsets. If an error is encountered, the read
 * preference will not be changed. */
int php_mongo_set_readpreference(mongo_read_preference *rp, char *read_preference, HashTable *tags TSRMLS_DC)
{
	mongo_read_preference tmp_rp;

	if (strcasecmp(read_preference, "primary") == 0) {
		if (tags && zend_hash_num_elements(tags)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "You can't use read preference tags with a read preference of PRIMARY");
			return 0;
		}
		tmp_rp.type = MONGO_RP_PRIMARY;
	} else if (strcasecmp(read_preference, "primaryPreferred") == 0) {
		tmp_rp.type = MONGO_RP_PRIMARY_PREFERRED;
	} else if (strcasecmp(read_preference, "secondary") == 0) {
		tmp_rp.type = MONGO_RP_SECONDARY;
	} else if (strcasecmp(read_preference, "secondaryPreferred") == 0) {
		tmp_rp.type = MONGO_RP_SECONDARY_PREFERRED;
	} else if (strcasecmp(read_preference, "nearest") == 0) {
		tmp_rp.type = MONGO_RP_NEAREST;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The value '%s' is not valid as read preference type", read_preference);
		return 0;
	}

	tmp_rp.tagsets = NULL;
	tmp_rp.tagset_count = 0;

	if (tags && zend_hash_num_elements(tags)) {
		if (!php_mongo_use_tagsets(&tmp_rp, tags TSRMLS_CC)) {
			return 0;
		}
	}

	mongo_read_preference_replace(&tmp_rp, rp);
	mongo_read_preference_dtor(&tmp_rp);

	return 1;
}

int php_mongo_trigger_error_on_command_failure(mongo_connection *connection, zval *document TSRMLS_DC)
{
	zval **tmpvalue;

	if (Z_TYPE_P(document) != IS_ARRAY) {
		zend_throw_exception(mongo_ce_ResultException, strdup("Unknown error executing command (empty document returned)"), 1 TSRMLS_CC);
		return FAILURE;
	}

	if (zend_hash_find(Z_ARRVAL_P(document), "ok", strlen("ok") + 1, (void **) &tmpvalue) == SUCCESS) {
		if ((Z_TYPE_PP(tmpvalue) == IS_LONG && Z_LVAL_PP(tmpvalue) < 1) || (Z_TYPE_PP(tmpvalue) == IS_DOUBLE && Z_DVAL_PP(tmpvalue) < 1)) {
			zval **tmp, *error_doc, *exception;
			char *message;
			long code;

			if (zend_hash_find(Z_ARRVAL_P(document), "errmsg", strlen("errmsg") + 1, (void **) &tmp) == SUCCESS) {
				convert_to_string_ex(tmp);
				message = Z_STRVAL_PP(tmp);
			} else {
				message = estrdup("Unknown error executing command");
			}

			if (zend_hash_find(Z_ARRVAL_P(document), "code", strlen("code") + 1, (void **) &tmp) == SUCCESS) {
				convert_to_long_ex(tmp);
				code = Z_LVAL_PP(tmp);
			} else {
				code = 2;
			}

			exception = php_mongo_cursor_throw(mongo_ce_ResultException, connection, code TSRMLS_CC, "%s", message);

			/* Since document may be a return_value (if this function is invoked
			 * through php_mongo_trigger_error_on_gle() and not findAndModify),
			 * copy it to a new zval before updating the exception property. */
			MAKE_STD_ZVAL(error_doc);
			array_init(error_doc);
			zend_hash_copy(Z_ARRVAL_P(error_doc), Z_ARRVAL_P(document), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
			zend_update_property(Z_OBJCE_P(exception), exception, "document", strlen("document"), document TSRMLS_CC);
			zval_ptr_dtor(&error_doc);

			return FAILURE;
		}
	}
	return SUCCESS;
}

int php_mongo_trigger_error_on_gle(mongo_connection *connection, zval *document TSRMLS_DC)
{
	zval **err;
	zend_class_entry *exception_ce = mongo_ce_WriteConcernException;

	/* Check if the GLE command itself failed */
	if (php_mongo_trigger_error_on_command_failure(connection, document TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	/* Check for an error message in the "err" field. Technically, a non-null
	 * value indicates an error, but we'll check for a non-empty string. */
	if (
		zend_hash_find(Z_ARRVAL_P(document), "err", strlen("err") + 1, (void**) &err) == SUCCESS &&
		Z_TYPE_PP(err) == IS_STRING && Z_STRLEN_PP(err) > 0
	) {
		zval *error_doc, *exception, **code_z, **wnote_z;
		/* Default error code from handle_error() in cursor.c */
		long code = 4;

		/* Check for the error code in the "code" field. */
		if (zend_hash_find(Z_ARRVAL_P(document), "code", strlen("code") + 1, (void **) &code_z) == SUCCESS) {
			convert_to_long_ex(code_z);
			code = Z_LVAL_PP(code_z);
		}

		/* If additional information is found in the "wnote" field, include it
		 * in the exception message. Otherwise, just use "err". */
		if (
			zend_hash_find(Z_ARRVAL_P(document), "wnote", strlen("wnote") + 1, (void**) &wnote_z) == SUCCESS &&
			Z_TYPE_PP(wnote_z) == IS_STRING && Z_STRLEN_PP(wnote_z) > 0
		) {
			exception = php_mongo_cursor_throw(exception_ce, connection, code TSRMLS_CC, "%s: %s", Z_STRVAL_PP(err), Z_STRVAL_PP(wnote_z));
		} else {
			exception = php_mongo_cursor_throw(exception_ce, connection, code TSRMLS_CC, "%s", Z_STRVAL_PP(err));
		}

		/* Since document is a return_value (thanks to MONGO_METHOD stuff), copy
		 * it to a new zval before updating the exception property. */
		MAKE_STD_ZVAL(error_doc);
		array_init(error_doc);
		zend_hash_copy(Z_ARRVAL_P(error_doc), Z_ARRVAL_P(document), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		zend_update_property(mongo_ce_WriteConcernException, exception, "document", strlen("document"), error_doc TSRMLS_CC);
		zval_ptr_dtor(&error_doc);

		return FAILURE;
	}

	return SUCCESS;
}
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
