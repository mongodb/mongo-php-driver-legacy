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
#include <ext/standard/php_smart_str.h>

#include "php_mongo.h"
#include "collection.h"
#include "command_cursor.h"
#include "cursor.h"
#include "cursor_shared.h"
#include "bson.h"
#include "db.h"
#include "types/code.h"
#include "types/db_ref.h"
#include "db.h"
#include "mcon/manager.h"
#include "mcon/utils.h"
#include "mcon/connections.h"
#include "log_stream.h"
#include "api/write.h"
#include "api/wire_version.h"
#include "contrib/php-json.h"

#undef fsync

extern zend_class_entry *mongo_ce_MongoClient, *mongo_ce_DB, *mongo_ce_Cursor;
extern zend_class_entry *mongo_ce_CommandCursor;
extern zend_class_entry *mongo_ce_Code, *mongo_ce_Exception, *mongo_ce_ResultException;
extern zend_class_entry *mongo_ce_CursorException;

extern zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

zend_class_entry *mongo_ce_Collection = NULL;

static int is_gle_op(zval *coll, zval *options, mongo_server_options *server_options, int silent TSRMLS_DC);
static void do_gle_op(mongo_con_manager *manager, mongo_connection *connection, zval *cursor_z, mongo_buffer *buf, zval *return_value TSRMLS_DC);
static zval* append_getlasterror(zval *coll, mongo_buffer *buf, zval *options, mongo_connection *connection TSRMLS_DC);
static char *to_index_string(zval *zkeys, int *key_len TSRMLS_DC);

/**
 * Unsets an option if it exists.
 *
 * @param zval *options
 * @param const char *name
 */
#define DELETE_OPTION_IF_EXISTS(options, name) do { \
	zval **tmp_option; \
	if (zend_hash_find(HASH_P(options), name, strlen(name) + 1, (void**)&tmp_option) == SUCCESS) { \
		zend_hash_del(HASH_P(options), name, strlen(name) + 1); \
	} \
} while(0);

static int is_valid_collectionname(char *colname, int colname_len TSRMLS_DC)
{
	if (colname_len == 0) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "Collection name cannot be empty");
		return 0;
	}

	if (
		/* strchr(colname, '$') != 0 || — we can not exclude this as we need it to run commands */
		memchr(colname, '\0', colname_len) != 0
	) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "Collection name cannot contain null bytes: %s\\0...", colname);
		return 0;
	}

	return 1;
}


void php_mongo_collection_construct(zval *this, zval *parent, char *name_str, int name_len TSRMLS_DC)
{
	zval *name, *zns, *w, *wtimeout;
	mongo_collection *c;
	mongo_db *db;
	char *ns;

	/* check for empty and invalid collection names */
	if (!is_valid_collectionname(name_str, name_len TSRMLS_CC)) {
		return;
	}

	c = (mongo_collection*)zend_object_store_get_object(this TSRMLS_CC);

	db = (mongo_db*)zend_object_store_get_object(parent TSRMLS_CC);
	if (!(db->name)) {
		zend_throw_exception(mongo_ce_Exception, "The MongoDB object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
		return;
	}

	c->link = db->link;
	zval_add_ref(&db->link);

	c->parent = parent;
	zval_add_ref(&parent);

	MAKE_STD_ZVAL(name);
	ZVAL_STRINGL(name, name_str, name_len, 1);
	c->name = name;

	spprintf(&ns, 0, "%s.%s", Z_STRVAL_P(db->name), Z_STRVAL_P(name));

	MAKE_STD_ZVAL(zns);
	ZVAL_STRING(zns, ns, 0);
	c->ns = zns;
	mongo_read_preference_copy(&db->read_pref, &c->read_pref);

	w = zend_read_property(mongo_ce_DB, parent, "w", strlen("w"), NOISY TSRMLS_CC);
	if (Z_TYPE_P(w) == IS_STRING) {
		zend_update_property_string(mongo_ce_Collection, this, "w", strlen("w"), Z_STRVAL_P(w) TSRMLS_CC);
	} else {
		convert_to_long(w);
		zend_update_property_long(mongo_ce_Collection, this, "w", strlen("w"), Z_LVAL_P(w) TSRMLS_CC);
	}

	wtimeout = zend_read_property(mongo_ce_DB, parent, "wtimeout", strlen("wtimeout"), NOISY TSRMLS_CC);
	convert_to_long(wtimeout);
	zend_update_property_long(mongo_ce_Collection, this, "wtimeout", strlen("wtimeout"), Z_LVAL_P(wtimeout) TSRMLS_CC);
}

/* {{{ proto MongoCollection MongoCollection::__construct(MongoDB db, string name)
   Initializes a new MongoCollection */
PHP_METHOD(MongoCollection, __construct)
{
	zval *db;
	char *name_str;
	int name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &db, mongo_ce_DB, &name_str, &name_len) == FAILURE) {
		zval *object = getThis();
		ZVAL_NULL(object);
		return;
	}

	php_mongo_collection_construct(getThis(), db, name_str, name_len TSRMLS_CC);
}
/* }}} */

/* {{{ proto string MongoCollection::__toString()
   Returns the full namespace for this collection (includes database name) */
PHP_METHOD(MongoCollection, __toString)
{
	mongo_collection *c;

	PHP_MONGO_GET_COLLECTION(getThis());
	RETURN_ZVAL(c->ns, 1, 0);
}
/* }}} */

/* {{{ proto string MongoCollection::getName()
   Returns the collection name */
PHP_METHOD(MongoCollection, getName)
{
	mongo_collection *c;

	PHP_MONGO_GET_COLLECTION(getThis());
	RETURN_ZVAL(c->name, 1, 0);
}
/* }}} */

/* {{{ proto bool MongoCollection::getSlaveOkay()
   Returns the slaveOkay flag for this collection */
PHP_METHOD(MongoCollection, getSlaveOkay)
{
	mongo_collection *c;

	PHP_MONGO_GET_COLLECTION(getThis());
	RETURN_BOOL(c->read_pref.type != MONGO_RP_PRIMARY);
}
/* }}} */

/* {{{ proto bool MongoCollection::setSlaveOkay([bool slave_okay = true])
   Sets the slaveOkay flag for this collection and returns the previous value */
PHP_METHOD(MongoCollection, setSlaveOkay)
{
	zend_bool slave_okay = 1;
	mongo_collection *c;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &slave_okay) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());

	RETVAL_BOOL(c->read_pref.type != MONGO_RP_PRIMARY);
	c->read_pref.type = slave_okay ? MONGO_RP_SECONDARY_PREFERRED : MONGO_RP_PRIMARY;
}
/* }}} */

/* {{{ proto array MongoCollection::getReadPreference()
   Returns an array describing the read preference for this collection. Tag sets will be included if available. */
PHP_METHOD(MongoCollection, getReadPreference)
{
	mongo_collection *c;
	PHP_MONGO_GET_COLLECTION(getThis());

	array_init(return_value);
	add_assoc_string(return_value, "type", mongo_read_preference_type_to_name(c->read_pref.type), 1);
	php_mongo_add_tagsets(return_value, &c->read_pref);
}
/* }}} */

/* {{{ proto bool MongoCollection::setReadPreference(string read_preference [, array tags ])
   Sets the read preference for this collection */
PHP_METHOD(MongoCollection, setReadPreference)
{
	char *read_preference;
	int   read_preference_len;
	mongo_collection *c;
	HashTable  *tags = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|h", &read_preference, &read_preference_len, &tags) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());

	if (php_mongo_set_readpreference(&c->read_pref, read_preference, tags TSRMLS_CC)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ array MongoCollection::getWriteConcern()
 * Get the MongoCollection write concern. */
PHP_METHOD(MongoCollection, getWriteConcern)
{
	zval *write_concern, *wtimeout;

	if (zend_parse_parameters_none()) {
		return;
	}

	write_concern = zend_read_property(mongo_ce_DB, getThis(), "w", strlen("w"), 0 TSRMLS_CC);
	wtimeout      = zend_read_property(mongo_ce_DB, getThis(), "wtimeout", strlen("wtimeout"), 0 TSRMLS_CC);
	Z_ADDREF_P(write_concern);
	Z_ADDREF_P(wtimeout);

	array_init(return_value);
	add_assoc_zval(return_value, "w", write_concern);
	add_assoc_zval(return_value, "wtimeout", wtimeout);
}
/* }}} */

/* {{{ bool MongoCollection::setWriteConcern(mixed w [, int wtimeout])
 * Set the MongoCollection write concern. */
PHP_METHOD(MongoCollection, setWriteConcern)
{
	zval *write_concern;
	long  wtimeout;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", &write_concern, &wtimeout) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(write_concern) == IS_LONG) {
		zend_update_property_long(mongo_ce_Collection, getThis(), "w", strlen("w"), Z_LVAL_P(write_concern) TSRMLS_CC);
	} else if (Z_TYPE_P(write_concern) == IS_STRING) {
		zend_update_property_stringl(mongo_ce_Collection, getThis(), "w", strlen("w"), Z_STRVAL_P(write_concern), Z_STRLEN_P(write_concern) TSRMLS_CC);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects parameter 1 to be an string or integer, %s given", zend_get_type_by_const(Z_TYPE_P(write_concern)));
		RETURN_FALSE;
	}

	if (ZEND_NUM_ARGS() > 1) {
		zend_update_property_long(mongo_ce_Collection, getThis(), "wtimeout", strlen("wtimeout"), wtimeout TSRMLS_CC);
	}

	RETURN_TRUE;
}
/* }}} */

void php_mongocollection_drop(zval *collection, zval *return_value TSRMLS_DC)
{
	zval *cmd, *retval;
	mongo_collection *c;
	mongo_db *db;

	PHP_MONGO_GET_COLLECTION(collection);
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_zval(cmd, "drop", c->name);
	zval_add_ref(&c->name);

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);
	zval_ptr_dtor(&cmd);

	if (retval) {
		RETURN_ZVAL(retval, 0, 1);
	}
}

/* {{{ proto array MongoCollection::drop()
   Drops the current collection and returns the database response */
PHP_METHOD(MongoCollection, drop)
{
	php_mongocollection_drop(getThis(), return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto array MongoCollection::validate([bool scan_data])
   Validates the current collection, optionally include the data, and returns the database response */
PHP_METHOD(MongoCollection, validate)
{
	zval *cmd, *retval;
	zend_bool scan_data = 0;
	mongo_collection *c;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &scan_data) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_string(cmd, "validate", Z_STRVAL_P(c->name), 1);
	add_assoc_bool(cmd, "full", scan_data);

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);
	zval_ptr_dtor(&cmd);

	if (retval) {
		RETURN_ZVAL(retval, 0, 1);
	}
}
/* }}} */

/* This should probably be split into two methods... right now appends the
 * getlasterror query to the buffer and alloc & inits the cursor zval. */
static zval* append_getlasterror(zval *coll, mongo_buffer *buf, zval *options, mongo_connection *connection TSRMLS_DC)
{
	zval *cmd_ns_z, *cmd, *cursor_z, *timeout_p;
	char *cmd_ns, *w_str = NULL;
	mongo_cursor *cursor;
	mongo_collection *c = (mongo_collection*)zend_object_store_get_object(coll TSRMLS_CC);
	mongo_db *db = (mongo_db*)zend_object_store_get_object(c->parent TSRMLS_CC);
	int response, w = 0, fsync = 0, journal = 0, timeout = -1;
	mongoclient *link = (mongoclient*) zend_object_store_get_object(c->link TSRMLS_CC);
	int max_document_size = connection->max_bson_size;
	int max_message_size = connection->max_message_size;

	mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror");

	timeout_p = zend_read_static_property(mongo_ce_Cursor, "timeout", strlen("timeout"), NOISY TSRMLS_CC);
	convert_to_long(timeout_p);

	/* The value hasn't been modified from what we registered it as originally, but we do
	 * need to set it to a real value */
	if (Z_LVAL_P(timeout_p) == PHP_MONGO_STATIC_CURSOR_TIMEOUT_NOT_SET_INITIALIZER) {
		timeout = link->servers->options.socketTimeoutMS;
	} else {
		/* The value was modified, bad user, bad user! Tell him its deprecated */
		timeout = Z_LVAL_P(timeout_p);
		php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'MongoCursor::$timeout' static property is deprecated, please call MongoCursor->timeout() instead");
	}

	/* Read the default_* properties from the link */
	if (link->servers->options.default_fsync != -1) {
		fsync = link->servers->options.default_fsync;
	}
	if (link->servers->options.default_journal != -1) {
		journal = link->servers->options.default_journal;
	}
	if (link->servers->options.default_w != -1) {
		w = link->servers->options.default_w;
	}
	if (link->servers->options.default_wstring != NULL) {
		w_str = link->servers->options.default_wstring;
	}

	/* This picks up the default "w" through the properties of MongoCollection
	 * and MongoDb, but only if w is still 1 - as otherwise it was perhaps
	 * overridden with the "w" (or "safe") option. */
	{
		zval *w_prop = zend_read_property(mongo_ce_Collection, coll, "w", strlen("w"), NOISY TSRMLS_CC);

		if (Z_TYPE_P(w_prop) == IS_STRING) {
			w_str = Z_STRVAL_P(w_prop);
		} else {
			convert_to_long(w_prop);
			if (Z_LVAL_P(w_prop) != 1) {
				w = Z_LVAL_P(w_prop);
				w_str = NULL;
			}
		}
	}

	/* Fetch all the options from the options array */
	if (options && IS_ARRAY_OR_OBJECT_P(options)) {
		zval **gle_pp = NULL, **fsync_pp, **timeout_pp, **journal_pp;

		/* First we try "w", and if that is not found we check for "safe" */
		if (zend_hash_find(HASH_P(options), "w", strlen("w") + 1, (void**) &gle_pp) == SUCCESS) {
			switch (Z_TYPE_PP(gle_pp)) {
				case IS_STRING:
					w_str = Z_STRVAL_PP(gle_pp);
					break;
				case IS_BOOL:
				case IS_LONG:
					w = Z_LVAL_PP(gle_pp); /* This is actually "wrong" for bools, but it works */
					break;
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "The value of the 'w' option either needs to be a integer or string");
			}
		} else if (zend_hash_find(HASH_P(options), "safe", strlen("safe") + 1, (void**) &gle_pp) == SUCCESS) {
			switch (Z_TYPE_PP(gle_pp)) {
				case IS_STRING:
					w_str = Z_STRVAL_PP(gle_pp);
					break;
				case IS_LONG:
					w = Z_LVAL_PP(gle_pp);
					break;
				case IS_BOOL:
					if (Z_BVAL_PP(gle_pp)) {
						/* If we already provided Write Concern, do not overwrite it with w=1 */
						if (!(w > 1 || w_str)) {
							w = 1;
						}
					} else {
						w = 0;
					}
					break;
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "The value of the 'safe' option either needs to be a integer or string");
			}
		}

		if (SUCCESS == zend_hash_find(HASH_P(options), "fsync", strlen("fsync") + 1, (void**) &fsync_pp)) {
			convert_to_boolean(*fsync_pp);
			fsync = Z_BVAL_PP(fsync_pp);
		}

		if (zend_hash_find(HASH_P(options), "j", strlen("j") + 1, (void**) &journal_pp) == SUCCESS) {
			convert_to_boolean(*journal_pp);
			journal = Z_BVAL_PP(journal_pp);
		}

		if (SUCCESS == zend_hash_find(HASH_P(options), "socketTimeoutMS", strlen("socketTimeoutMS") + 1, (void**) &timeout_pp)) {
			convert_to_long(*timeout_pp);
			timeout = Z_LVAL_PP(timeout_pp);
		} else if (SUCCESS == zend_hash_find(HASH_P(options), "timeout", strlen("timeout") + 1, (void**) &timeout_pp)) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'timeout' option is deprecated, please use 'socketTimeoutMS' instead");
			convert_to_long(*timeout_pp);
			timeout = Z_LVAL_PP(timeout_pp);
		}
	}

	/* fsync forces "w" to be atleast 1, so don't touch it if it's
	 * already set to something else above while parsing "w" (and
	 * "safe") */
	if (fsync && w == 0) {
		w = 1;
	}

	/* get "db.$cmd" zval */
	MAKE_STD_ZVAL(cmd_ns_z);
	spprintf(&cmd_ns, 0, "%s.$cmd", Z_STRVAL_P(db->name));
	ZVAL_STRING(cmd_ns_z, cmd_ns, 0);

	/* get {"getlasterror" : 1} zval */
	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_long(cmd, "getlasterror", 1);

	/* if we have either a string, or w > 1, then we need to add "w" and
	 * perhaps "wtimeout" to GLE */
	if (w_str || w > 1) {
		zval *wtimeout, **wtimeout_pp;

		if (w_str) {
			add_assoc_string(cmd, "w", w_str, 1);
			mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror: added w='%s'", w_str);
		} else {
			add_assoc_long(cmd, "w", w);
			mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror: added w=%d", w);
		}

		if (options && zend_hash_find(HASH_P(options), "wTimeoutMS", strlen("wTimeoutMS") + 1, (void **)&wtimeout_pp) == SUCCESS) {
			convert_to_long(*wtimeout_pp);
			add_assoc_long(cmd, "wtimeout", Z_LVAL_PP(wtimeout_pp));
			mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror: added wtimeout=%d (wTimeoutMS from options array)", Z_LVAL_PP(wtimeout_pp));
		} else if (options && zend_hash_find(HASH_P(options), "wtimeout", strlen("wtimeout") + 1, (void **)&wtimeout_pp) == SUCCESS) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'wtimeout' option is deprecated, please use 'wTimeoutMS' instead");
			convert_to_long(*wtimeout_pp);
			add_assoc_long(cmd, "wtimeout", Z_LVAL_PP(wtimeout_pp));
			mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror: added wtimeout=%d (wtimeout from options array)", Z_LVAL_PP(wtimeout_pp));
		} else {
			wtimeout = zend_read_property(mongo_ce_Collection, coll, "wtimeout", strlen("wtimeout"), NOISY TSRMLS_CC);
			convert_to_long(wtimeout);
			add_assoc_long(cmd, "wtimeout", Z_LVAL_P(wtimeout));
			mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror: added wtimeout=%d (from collection property)", Z_LVAL_P(wtimeout));
		}
	}

	if (fsync == 1) {
		add_assoc_bool(cmd, "fsync", 1);
		mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror: added fsync=1");
	}

	if (journal == 1) {
		add_assoc_bool(cmd, "j", 1);
		mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "append_getlasterror: added j=1");
	}

	/* get cursor */
	MAKE_STD_ZVAL(cursor_z);
	object_init_ex(cursor_z, mongo_ce_Cursor);

	cursor = (mongo_cursor*)zend_object_store_get_object(cursor_z TSRMLS_CC);
	php_mongocursor_create(cursor, c->link, Z_STRVAL_P(cmd_ns_z), Z_STRLEN_P(cmd_ns_z), cmd, NULL TSRMLS_CC);

	if (EG(exception)) {
		zval_ptr_dtor(&cursor_z);
		zval_ptr_dtor(&cmd_ns_z);
		zval_ptr_dtor(&cmd);
		return 0;
	}

	/* Make sure the "getLastError" also gets send to a primary. This should
	 * be refactored alongside with the getLastError redirection in
	 * db.c/MongoDB::command. The Cursor creation should be done through an
	 * init method otherwise a connection have to be requested twice. */
	mongo_manager_log(link->manager, MLOG_CON, MLOG_INFO, "forcing primary for getlasterror");
	php_mongo_cursor_force_primary(cursor);

	cursor->limit = -1;
	cursor->timeout = timeout;
	zval_ptr_dtor(&cursor->query);

	/* append the query */
	response = php_mongo_write_query(buf, cursor, max_document_size, max_message_size TSRMLS_CC);
	zval_ptr_dtor(&cmd_ns_z);

	mongo_log_stream_query(connection, cursor TSRMLS_CC);

	if (FAILURE == response) {
		zval_ptr_dtor(&cursor_z);
		return 0;
	}

	return cursor_z;
}

/* Returns a connection for the operation.
 * Connection flags (connection_flags) are MONGO_CON_TYPE_READ and MONGO_CON_TYPE_WRITE. */
mongo_connection* php_mongo_collection_get_server(mongoclient *link, int connection_flags TSRMLS_DC)
{
	mongo_connection *connection;
	char *error_message = NULL;

	if (!link) {
		zend_throw_exception(mongo_ce_Exception, "The MongoCollection object has not been correctly initialized by its constructor", 17 TSRMLS_CC);
		return NULL;
	}

	/* TODO: Fix better error message */
	if ((connection = mongo_get_read_write_connection(link->manager, link->servers, connection_flags, (char **) &error_message)) == NULL) {
		if (error_message) {
			php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 16 TSRMLS_CC, "Couldn't get connection: %s", error_message);
			free(error_message);
		} else {
			php_mongo_cursor_throw(mongo_ce_CursorException, NULL, 16 TSRMLS_CC, "Couldn't get connection");
		}
		return NULL;
	}

	return connection;
}

/* Wrapper for sending and wrapping in a safe op */
static int send_message(zval *this_ptr, mongo_connection *connection, mongo_buffer *buf, zval *options, zval *return_value TSRMLS_DC)
{
	int retval = 1;
	char *error_message = NULL;
	mongoclient *link;
	mongo_collection *c;

	c = (mongo_collection*)zend_object_store_get_object(this_ptr TSRMLS_CC);
	if (!c->ns) {
		zend_throw_exception(mongo_ce_Exception, "The MongoCollection object has not been correctly initialized by its constructor", 0 TSRMLS_CC);
		return 0;
	}

	link = (mongoclient*)zend_object_store_get_object((c->link) TSRMLS_CC);
	if (!link) {
		zend_throw_exception(mongo_ce_Exception, "The MongoCollection object has not been correctly initialized by its constructor", 17 TSRMLS_CC);
		return 0;
	}

	if (is_gle_op(this_ptr, options, &link->servers->options, NOISY TSRMLS_CC)) {
		zval *cursor = append_getlasterror(this_ptr, buf, options, connection TSRMLS_CC);
		if (cursor) {
			do_gle_op(link->manager, connection, cursor, buf, return_value TSRMLS_CC);
			zval_ptr_dtor(&cursor);
			retval = -1;
		} else {
			retval = 0;
		}
	} else if (link->manager->send(connection, &link->servers->options, buf->start, buf->pos - buf->start, (char **) &error_message) == -1) {
		/* TODO: Find out what to do with the error message here */
		free(error_message);
		retval = 0;
	} else {
		retval = 1;
	}
	return retval;
}

/* Determine if the operation should have a GLE command appended. This is based
 * on whether a write concern ("w" or "safe") or fsync/journal options are
 * specified.
 */
static int is_gle_op(zval *coll, zval *options, mongo_server_options *server_options, int silent TSRMLS_DC)
{
	int gle_op = 0, default_fsync, default_journal, coll_w = 0;
	zval *z_coll_w;

	/* Get the fsync/journal defaults from the MongoClient server options */
	default_fsync = server_options->default_fsync;
	default_journal = server_options->default_journal;

	/* Get the MongoCollection write concern instead of checking the MongoClient
	 * server options, since MongoCollection will have inherited any defaults
	 * through the MongoDB class. Additionally, this ensures that we respect a
	 * write concern set directly on the MongoCollection instance. */
	z_coll_w = zend_read_property(mongo_ce_Collection, coll, "w", strlen("w"), NOISY TSRMLS_CC);

	if (Z_TYPE_P(z_coll_w) == IS_STRING) {
		/* We don't actually care what the string is, only that it was specified */
		coll_w = 1;
	} else {
		convert_to_long(z_coll_w);
		coll_w = Z_LVAL_P(z_coll_w);
	}

	/* Then we check the options array that could overwrite the default */
	if (options && Z_TYPE_P(options) == IS_ARRAY) {
		zval **gle_pp, **fsync_pp, **journal_pp;

		/* Check for "w" in the options array. If it is not found, consult the
		 * "safe" option, followed by the MongoCollection write concern. */
		if (zend_hash_find(HASH_P(options), "w", strlen("w") + 1, (void**) &gle_pp) == SUCCESS) {
			switch (Z_TYPE_PP(gle_pp)) {
				case IS_STRING:
					gle_op = 1;
					break;
				case IS_BOOL:
				case IS_LONG:
					/* This is actually "wrong" for bools, but it works */
					if (Z_LVAL_PP(gle_pp) >= 1) {
						gle_op = 1;
					}
					break;
				default:
					if (silent == NOISY) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "The value of the 'w' option either needs to be a integer or string");
					}
			}
		} else if (zend_hash_find(HASH_P(options), "safe", strlen("safe") + 1, (void**) &gle_pp) == SUCCESS) {
			if (silent == NOISY) {
				php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'safe' option is deprecated, please use 'w' instead");
			}

			switch (Z_TYPE_PP(gle_pp)) {
				case IS_STRING:
					gle_op = 1;
					break;
				case IS_BOOL:
				case IS_LONG:
					/* This is actually "wrong" for bools, but it works */
					if (Z_LVAL_PP(gle_pp) >= 1) {
						gle_op = 1;
					}
					break;
				default:
					if (silent == NOISY) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "The value of the 'safe' option either needs to be a boolean or a string");
					}
			}
		} else if (coll_w >= 1) {
			gle_op = 1;
		}

		/* Check for "fsync" in the options array. If it is not found, respect
		 * the MongoClient "fsync" option. */
		if (zend_hash_find(HASH_P(options), "fsync", strlen("fsync") + 1, (void**)&fsync_pp) == SUCCESS) {
			convert_to_boolean_ex(fsync_pp);
			if (Z_BVAL_PP(fsync_pp)) {
				gle_op = 1;
			}
		} else if (default_fsync == 1) {
			gle_op = 1;
		}

		/* Check for "j" in the options array. If it is not found, respect the
		 * MongoClient "journal" option. */
		if (zend_hash_find(HASH_P(options), "j", strlen("j") + 1, (void**)&journal_pp) == SUCCESS) {
			convert_to_boolean_ex(journal_pp);
			if (Z_BVAL_PP(journal_pp)) {
				gle_op = 1;
			}
		} else if (default_journal == 1) {
			gle_op = 1;
		}
	} else {
		gle_op = (coll_w >= 1 || default_fsync == 1 || default_journal == 1);
	}

	if (silent == NOISY) {
		mongo_manager_log(MonGlo(manager), MLOG_IO, MLOG_FINE, "is_gle_op: %s", gle_op ? "yes" : "no");
	}
	return gle_op;
}

/* This wrapper temporarily turns off the exception throwing bit if it has been
 * set (by calling php_mongo_cursor_throw() before). We can't call
 * php_mongo_cursor_throw after deregister as it frees up bits of memory that
 * php_mongo_cursor_throw uses to construct its error message.
 *
 * Without the disabling of the exception bit and when a user defined error
 * handler is used on the PHP side, the notice would never been shown because
 * the exception bubbles up before the notice can actually be shown. By turning
 * the error handling mode to EH_NORMAL temporarily, we circumvent this
 * problem. */
static void connection_deregister_wrapper(mongo_con_manager *manager, mongo_connection *connection TSRMLS_DC)
{
	int orig_error_handling;

	/* Save EG/PG(error_handling) so that we can show log messages when we have
	 * already thrown an exception */
	orig_error_handling = EG(error_handling);
	EG(error_handling) = EH_NORMAL;

	mongo_manager_connection_deregister(manager, connection);

	EG(error_handling) = orig_error_handling;
}

static void do_gle_op(mongo_con_manager *manager, mongo_connection *connection, zval *cursor_z, mongo_buffer *buf, zval *return_value TSRMLS_DC)
{
	char *error_message;
	mongo_cursor *cursor;
	mongoclient *client;

	cursor = (mongo_cursor*)zend_object_store_get_object(cursor_z TSRMLS_CC);
	client = (mongoclient*)zend_object_store_get_object(cursor->zmongoclient TSRMLS_CC);
	cursor->connection = connection;

	if (-1 == manager->send(connection, &client->servers->options, buf->start, buf->pos - buf->start, (char **) &error_message)) {
		mongo_manager_log(manager, MLOG_IO, MLOG_WARN, "do_gle_op: sending data failed, removing connection %s", connection->hash);
		php_mongo_cursor_throw(mongo_ce_CursorException, connection, 16 TSRMLS_CC, "%s", error_message);
		connection_deregister_wrapper(manager, connection TSRMLS_CC);

		free(error_message);
		cursor->connection = NULL;
		return;
	}

	/* get reply */
	if (FAILURE == php_mongo_get_reply(cursor TSRMLS_CC)) {
		/* php_mongo_get_reply() throws exceptions */
		mongo_manager_connection_deregister(manager, connection);
		cursor->connection = NULL;
		return;
	}

	cursor->started_iterating = 1;

	php_mongocursor_load_current_element(cursor TSRMLS_CC);

	/* MongoCursor::next() threw an exception */
	if (EG(exception)) {
		cursor->connection = NULL;
		return;
	}

	/* Check if either the GLE command or the previous write operation failed */
	php_mongo_trigger_error_on_gle(cursor->connection, cursor->current TSRMLS_CC);
	RETVAL_ZVAL(cursor->current, 1, 0);

	cursor->connection = NULL;
	return;
}

int mongo_collection_insert_opcode(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *options, zval *write_options, zval *this_ptr, mongo_buffer *buf, char *namespace, int namespace_len, zval *document, zval *return_value TSRMLS_DC)
{
	int retval = 0;

	if (FAILURE == php_mongo_write_insert(buf, namespace, document, connection->max_bson_size, connection->max_message_size TSRMLS_CC)) {
		return 0;
	}

	mongo_log_stream_insert(connection, document, write_options TSRMLS_CC);

	/* retval == -1 means a GLE response was received, so send_message() has
	 * either set return_value or thrown an exception via do_gle_op(). */
	retval = send_message(this_ptr, connection, buf, write_options, return_value TSRMLS_CC);

	return retval;
}

void mongo_convert_write_api_return_to_legacy_retval(zval *return_value, php_mongo_write_types type, int write_concern TSRMLS_DC)
{
	zval **ok, **err, **errmsg, **n;

	if (write_concern < 1) {
		/* Not kidding.. Surpress any exception thrown for w=0 */
		zend_clear_exception(TSRMLS_C);
		convert_to_boolean(return_value);
		return;
	}
	if (SUCCESS == zend_hash_find(HASH_P(return_value), "ok", strlen("ok") + 1, (void**) &ok) && Z_TYPE_PP(ok) != IS_DOUBLE) {
		convert_to_double(*ok);
	}
	if (FAILURE == zend_hash_find(HASH_P(return_value), "err", strlen("err") + 1, (void**) &err)) {
		add_assoc_null(return_value, "err");
	}
	if (FAILURE == zend_hash_find(HASH_P(return_value), "errmsg", strlen("errmsg") + 1, (void**) &errmsg)) {
		add_assoc_null(return_value, "errmsg");
	}
	switch (type) {
		case MONGODB_API_COMMAND_INSERT:

			if (SUCCESS == zend_hash_find(HASH_P(return_value), "n", strlen("n") + 1, (void**) &n)) {
				/* "n" was always 0 for OP_INSERT gle. */
				convert_to_long(*n);
				Z_LVAL_PP(n) = 0;
			}
			break;

		case MONGODB_API_COMMAND_UPDATE: {
			int updatedExisting = 0;

			if (SUCCESS == zend_hash_find(HASH_P(return_value), "n", strlen("n") + 1, (void**) &n)) {
				zval **upserted;

				convert_to_long_ex(n);
				if (SUCCESS == zend_hash_find(HASH_P(return_value), "upserted", strlen("upserted") + 1, (void**) &upserted) && Z_TYPE_PP(upserted) == IS_ARRAY) {
					zval **upserted_index;

					if (SUCCESS == zend_hash_get_current_data(Z_ARRVAL_PP(upserted), (void **)&upserted_index)) {
						zval **upserted_id;

						/* upserted is an array of _ids in 2.6 */
						if (SUCCESS == zend_hash_find(HASH_PP(upserted_index), "_id", strlen("_id") + 1, (void**) &upserted_id)) {
							zval *id;

							MAKE_STD_ZVAL(id);
							MAKE_COPY_ZVAL(upserted_id, id);
							zend_hash_del(HASH_OF(return_value), "upserted", strlen("upserted") + 1);
							add_assoc_zval(return_value, "upserted", id);
						}
					}
				}
				else if (Z_LVAL_PP(n) > 0) {
					/* updatedExisting needs to be set to true when existing documents were modified */
					updatedExisting = 1;
				}
			}
			add_assoc_bool(return_value, "updatedExisting", updatedExisting);
		} break;

		case MONGODB_API_COMMAND_DELETE:
			break;
	}
}

void mongo_apply_implicit_write_options(php_mongo_write_options *write_options, mongo_server_options *server_options, zval *collection TSRMLS_DC)
{
	if (write_options->fsync == -1) {
		write_options->fsync = server_options->default_fsync;
	}
	if (write_options->j == -1) {
		write_options->j = server_options->default_journal;
	}
	if (write_options->wtimeout == -1) {
		zval *wtimeout_prop;

		write_options->wtimeout = server_options->default_wtimeout;

		wtimeout_prop = zend_read_property(mongo_ce_Collection, collection, "wtimeout", strlen("wtimeout"), NOISY TSRMLS_CC);
		convert_to_long(wtimeout_prop);
		if (Z_LVAL_P(wtimeout_prop) != PHP_MONGO_DEFAULT_WTIMEOUT) {
			write_options->wtimeout = Z_LVAL_P(wtimeout_prop);
		}
	}

	if (write_options->wtype == -1) {
		zval *w_prop = zend_read_property(mongo_ce_Collection, collection, "w", strlen("w"), NOISY TSRMLS_CC);

		if (Z_TYPE_P(w_prop) == IS_LONG || Z_TYPE_P(w_prop) == IS_BOOL) {
			if (Z_LVAL_P(w_prop) == 1) {
				if (server_options->default_w != -1) {
					write_options->write_concern.w = server_options->default_w;
					write_options->wtype = 1;
				} else if (server_options->default_wstring != NULL) {
					write_options->write_concern.wstring = server_options->default_wstring;
					write_options->wtype = 2;
				} else {
					write_options->write_concern.w = Z_LVAL_P(w_prop);
					write_options->wtype = 1;
				}
			} else {
				write_options->write_concern.w = Z_LVAL_P(w_prop);
				write_options->wtype = 1;
			}
		} else {
			convert_to_string(w_prop);
			write_options->write_concern.wstring = Z_STRVAL_P(w_prop);
			write_options->wtype = 2;
		}
	}

}

/*
 * Returns the socket read timeout (in ms) from the specified write options
 * ('socketTimeoutMS', with a deprecated fallback to 'timeout').
 */
int mongo_get_socket_read_timeout(mongo_server_options *server_options, zval *z_write_options TSRMLS_DC)
{
	if (z_write_options && Z_TYPE_P(z_write_options) == IS_ARRAY) {
		zval **timeout_pp;

		if (SUCCESS == zend_hash_find(HASH_P(z_write_options), "socketTimeoutMS", strlen("socketTimeoutMS") + 1, (void**) &timeout_pp)) {
			convert_to_long(*timeout_pp);
			return Z_LVAL_PP(timeout_pp);
		} else if (SUCCESS == zend_hash_find(HASH_P(z_write_options), "timeout", strlen("timeout") + 1, (void**) &timeout_pp)) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The 'timeout' option is deprecated, please use 'socketTimeoutMS' instead");
			convert_to_long(*timeout_pp);
			return Z_LVAL_PP(timeout_pp);
		}
	}

	return server_options->socketTimeoutMS;
}

int mongo_collection_delete_api(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *server_options, int socket_read_timeout, php_mongo_write_delete_args *delete_options, php_mongo_write_options *write_options, char *dbname, zval *collection, zval *return_value TSRMLS_DC)
{
	char *command_ns;
	char *error_message;
	int retval = 0;
	int bytes_written = 0;
	int request_id;
	mongo_buffer buf;
	mongo_collection *c = (mongo_collection*)zend_object_store_get_object(collection TSRMLS_CC);

	spprintf(&command_ns, 0, "%s.$cmd", dbname);

	CREATE_BUF(buf, INITIAL_BUF_SIZE);
	request_id = php_mongo_api_delete_single(&buf, command_ns, Z_STRVAL_P(c->name), delete_options, write_options, connection TSRMLS_CC);

	efree(command_ns);

	if (request_id == 0) {
		/* The document is greater then allowed limits, exception already thrown */
		efree(buf.start);
		return 0;
	}

	bytes_written = manager->send(connection, server_options, buf.start, buf.pos - buf.start, &error_message);

	if (bytes_written < 1) {
		/* Didn't write anything, something bad must have happened */
		free(error_message);
		efree(buf.start);
		return 0;
	}

	array_init(return_value);
	retval = php_mongo_api_get_reply(manager, connection, server_options, socket_read_timeout, request_id, &return_value TSRMLS_CC);
	efree(buf.start);

	if (retval != 0) {
		mongo_manager_connection_deregister(manager, connection);
		if (write_options->wtype == 1 && write_options->write_concern.w < 1) {
			/* Clear out exception when w=0 */
			zend_clear_exception(TSRMLS_C);
			convert_to_boolean(return_value);
			return 0;
		}
		return 0;
	}

	if (php_mongo_api_raise_exception_on_command_failure(connection, return_value TSRMLS_CC)) {
		return 0;
	}

	if (php_mongo_api_raise_exception_on_write_failure(connection, return_value TSRMLS_CC)) {
		return 0;
	}

	return 1;
}

int mongo_collection_update_api(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *server_options, int socket_read_timeout, php_mongo_write_update_args *update_options, php_mongo_write_options *write_options, char *dbname, zval *collection, zval *return_value TSRMLS_DC)
{
	char *command_ns;
	char *error_message;
	int retval = 0;
	int bytes_written = 0;
	int request_id;
	mongo_buffer buf;
	mongo_collection *c = (mongo_collection*)zend_object_store_get_object(collection TSRMLS_CC);

	spprintf(&command_ns, 0, "%s.$cmd", dbname);

	CREATE_BUF(buf, INITIAL_BUF_SIZE);
	request_id = php_mongo_api_update_single(&buf, command_ns, Z_STRVAL_P(c->name), update_options, write_options, connection TSRMLS_CC);

	efree(command_ns);

	if (request_id == 0) {
		/* The document is greater then allowed limits, exception already thrown */
		efree(buf.start);
		return 0;
	}

	bytes_written = manager->send(connection, server_options, buf.start, buf.pos - buf.start, &error_message);

	if (bytes_written < 1) {
		/* Didn't write anything, something bad must have happened */
		free(error_message);
		efree(buf.start);
		return 0;
	}

	array_init(return_value);
	retval = php_mongo_api_get_reply(manager, connection, server_options, socket_read_timeout, request_id, &return_value TSRMLS_CC);
	efree(buf.start);

	if (retval != 0) {
		mongo_manager_connection_deregister(manager, connection);
		if (write_options->wtype == 1 && write_options->write_concern.w < 1) {
			/* Clear out exception when w=0 */
			zend_clear_exception(TSRMLS_C);
			convert_to_boolean(return_value);
			return 0;
		}
		return 0;
	}

	if (php_mongo_api_raise_exception_on_command_failure(connection, return_value TSRMLS_CC)) {
		return 0;
	}

	if (php_mongo_api_raise_exception_on_write_failure(connection, return_value TSRMLS_CC)) {
		return 0;
	}

	return 1;
}

/* Returns 0 on failure, throwing exception?
 * Returns 1 on success, setting zval return_value to the return document
 */
int mongo_collection_insert_api(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *server_options, int socket_read_timeout, php_mongo_write_options *write_options, char *dbname, zval *collection, zval *document, zval *return_value TSRMLS_DC)
{
	char *command_ns;
	char *error_message;
	int retval = 0;
	int bytes_written = 0;
	int request_id;
	mongo_buffer buf;
	mongo_collection *c = (mongo_collection*)zend_object_store_get_object(collection TSRMLS_CC);

	spprintf(&command_ns, 0, "%s.$cmd", dbname);

	CREATE_BUF(buf, INITIAL_BUF_SIZE);
	request_id = php_mongo_api_insert_single(&buf, command_ns, Z_STRVAL_P(c->name), document, write_options, connection TSRMLS_CC);

	efree(command_ns);

	if (request_id == 0) {
		/* The document is greater then allowed limits, exception already thrown */
		efree(buf.start);
		return 0;
	}

	bytes_written = manager->send(connection, server_options, buf.start, buf.pos - buf.start, &error_message);

	if (bytes_written < 1) {
		/* Didn't write anything, something bad must have happened */
		free(error_message);
		efree(buf.start);
		return 0;
	}

	array_init(return_value);
	retval = php_mongo_api_get_reply(manager, connection, server_options, socket_read_timeout, request_id, &return_value TSRMLS_CC);
	efree(buf.start);

	if (retval != 0) {
		mongo_manager_connection_deregister(manager, connection);
		/* Exception already thrown */
		return 0;
	}

	if (php_mongo_api_raise_exception_on_command_failure(connection, return_value TSRMLS_CC)) {
		return 0;
	}

	if (php_mongo_api_raise_exception_on_write_failure(connection, return_value TSRMLS_CC)) {
		return 0;
	}

	return 1;
}

static void php_mongo_collection_insert(zval *z_collection, zval *document, zval *z_write_options, zval *return_value TSRMLS_DC)
{
	mongoclient *link;
	mongo_collection *c;
	mongo_connection *connection;
	int is_acknowledged, supports_command, supports_opcode;

	PHP_MONGO_GET_COLLECTION(z_collection);
	PHP_MONGO_GET_LINK(c->link);

	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	is_acknowledged = is_gle_op(z_collection, z_write_options, &link->servers->options, QUIET TSRMLS_CC);
	supports_command = php_mongo_api_connection_supports_feature(connection, PHP_MONGO_API_WRITE_API);
	supports_opcode = php_mongo_api_connection_supports_feature(connection, PHP_MONGO_API_RELEASE_2_4_AND_BEFORE);

	/* Allow unacknowledged writes to be routed through legacy opcodes for
	 * performance, unless the server no longer supports the legacy opcode.
	 * This applies to the insert(), remove(), save(), and update() methods. */
	if (supports_command && (is_acknowledged || !supports_opcode)) {
		php_mongo_write_options write_options = {-1, {-1}, -1, -1, -1, -1};
		int retval;
		mongo_db *db;
		int socket_read_timeout = 0;

		PHP_MONGO_GET_DB(c->parent);

		mongo_apply_implicit_write_options(&write_options, &link->servers->options, z_collection TSRMLS_CC);
		php_mongo_api_write_options_from_zval(&write_options, z_write_options TSRMLS_CC);
		socket_read_timeout = mongo_get_socket_read_timeout(&link->servers->options, z_write_options TSRMLS_CC);

		retval = mongo_collection_insert_api(link->manager, connection, &link->servers->options, socket_read_timeout, &write_options, Z_STRVAL_P(db->name), z_collection, document, return_value TSRMLS_CC);

		if (retval) {
			/* Adds random "err", "code", "errmsg" empty fields to be compatible with
			 * old-style return values */
			mongo_convert_write_api_return_to_legacy_retval(return_value, MONGODB_API_COMMAND_INSERT, write_options.wtype == 1 ? write_options.write_concern.w : 1 TSRMLS_CC);
		} else {
			if (write_options.wtype == 1 && write_options.write_concern.w < 1) {
				zend_clear_exception(TSRMLS_C);
				convert_to_boolean(return_value);
				return;
			}
		}
		return;
	} else if (supports_opcode) {
		int retval;
		mongo_buffer buf;

		CREATE_BUF(buf, INITIAL_BUF_SIZE);
		retval = mongo_collection_insert_opcode(link->manager, connection, &link->servers->options, z_write_options, z_collection, &buf, Z_STRVAL_P(c->ns), Z_STRLEN_P(c->ns), document, return_value TSRMLS_CC);

		/* retval == -1 means a GLE response was received, so send_message() has
		* either set return_value or thrown an exception via do_gle_op(). */
		if (retval != -1) {
			RETVAL_BOOL(retval);
		}
		efree(buf.start);
	} else {
		zend_throw_exception_ex(mongo_ce_Exception, 0 TSRMLS_CC, "Cannot determine how to write documents to the server");
		return;
	}
}

/* {{{ proto bool|array MongoCollection::insert(array|object document [, array options])
   Insert a document into the collection and return the database response if
   the write concern is >= 1. Otherwise, boolean true is returned if the
   document is not empty. */
PHP_METHOD(MongoCollection, insert)
{
	zval *document, *z_write_options = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &document, &z_write_options) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(1, document);

	php_mongo_collection_insert(getThis(), document, z_write_options, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool|array MongoCollection::batchInsert(array documents [, array options])
   Insert an array of documents and return the database response if the write
   concern is >= 1. Otherwise, a boolean value is returned indicating whether
   the batch was successfully sent. */
PHP_METHOD(MongoCollection, batchInsert)
{
	zval *docs, *options = NULL;
	mongo_collection *c;
	mongo_connection *connection;
	mongo_buffer buf;
	int bit_opts = 0;
	int retval, count;
	mongoclient *link;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|z/", &docs, &options) == FAILURE) {
		return;
	}

	/* Options are only supported in the new-style, ie: an array of "named
	 * parameters": array("continueOnError" => true); */
	if (options) {
		zval **continue_on_error = NULL;

		if (zend_hash_find(HASH_P(options), "continueOnError", strlen("continueOnError") + 1, (void**)&continue_on_error) == SUCCESS) {
			convert_to_boolean_ex(continue_on_error);
			bit_opts = Z_BVAL_PP(continue_on_error) << 0;
		}

		Z_ADDREF_P(options);
	} else {
		MAKE_STD_ZVAL(options);
		array_init(options);
	}

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_LINK(c->link);

	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		zval_ptr_dtor(&options);
		RETURN_FALSE;
	}

	CREATE_BUF(buf, INITIAL_BUF_SIZE);

	count = php_mongo_write_batch_insert(&buf, Z_STRVAL_P(c->ns), bit_opts, docs, connection->max_bson_size, connection->max_message_size TSRMLS_CC);
	if (count == FAILURE) {
		efree(buf.start);
		zval_ptr_dtor(&options);
		return;
	}
	if (count == 0) {
		/* Empty array, or just scalar data */
		zend_throw_exception(mongo_ce_Exception, "No write ops were included in the batch", 16 TSRMLS_CC);
		efree(buf.start);
		zval_ptr_dtor(&options);
		return;
	}

	mongo_log_stream_batchinsert(connection, docs, options, bit_opts TSRMLS_CC);

	/* retval == -1 means a GLE response was received, so send_message() has
	 * either set return_value or thrown an exception via do_gle_op(). */
	retval = send_message(this_ptr, connection, &buf, options, return_value TSRMLS_CC);
	if (retval != -1) {
		RETVAL_BOOL(retval);
	}

	efree(buf.start);
	zval_ptr_dtor(&options);
}
/* }}} */

int php_mongo_collection_find(mongo_cursor *cursor, mongo_collection *c, zval *query, zval *fields TSRMLS_DC)
{
	mongo_read_preference_replace(&c->read_pref, &cursor->read_pref);
	return php_mongocursor_create(cursor, c->link, Z_STRVAL_P(c->ns), Z_STRLEN_P(c->ns), query, fields TSRMLS_CC);
}


/* {{{ proto array MongoCollection::find([array|object criteria [, array|object return_fields]])
   Query this collection for documents matching $criteria and use $return_fields
   as the projection. Return a MongoCursor for the result set. */
PHP_METHOD(MongoCollection, find)
{
	zval *query = 0, *fields = 0;
	mongo_collection *c;
	mongo_cursor *cursor;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &query, &fields) == FAILURE) {
		return;
	}

	MUST_BE_ARRAY_OR_OBJECT(1, query);
	MUST_BE_ARRAY_OR_OBJECT(2, fields);

	PHP_MONGO_GET_COLLECTION(getThis());

	object_init_ex(return_value, mongo_ce_Cursor);
	cursor = (mongo_cursor*)zend_object_store_get_object(return_value TSRMLS_CC);

	php_mongo_collection_find(cursor, c, query, fields TSRMLS_CC);
}
/* }}} */

void php_mongo_collection_findone(zval *this_ptr, zval *query, zval *fields, zval *options, zval *return_value TSRMLS_DC)
{
	mongo_cursor *cursor;
	mongo_collection *c;
	zval *zcursor = NULL;

	PHP_MONGO_GET_COLLECTION(getThis());

	MAKE_STD_ZVAL(zcursor);
	object_init_ex(zcursor, mongo_ce_Cursor);
	cursor = (mongo_cursor*)zend_object_store_get_object(zcursor TSRMLS_CC);

	if (php_mongo_collection_find(cursor, c, query, fields TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&zcursor);
		return;
	}

	/* Set limit to 1 */
	php_mongo_cursor_set_limit(cursor, -1);

	if (options) {
		HashTable *hindex = HASH_P(options);
		HashPosition pointer;
		zval **data;
		char *key;
		uint key_type, index_key_len;
		ulong index;

		for (
			zend_hash_internal_pointer_reset_ex(hindex, &pointer);
			zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(hindex, &pointer)
		) {
			key_type = zend_hash_get_current_key_ex(hindex, &key, &index_key_len, &index, NO_DUP, &pointer);

			if (key_type == HASH_KEY_IS_LONG) {
				continue;
			}

			if (zend_binary_strcasecmp(key, index_key_len, "maxTimeMS", strlen("maxTimeMS") + 1) == 0) {
				convert_to_long_ex(data);
				if ( ! php_mongo_cursor_add_option(cursor, "$maxTimeMS", *data TSRMLS_CC)) {
					goto cleanup_on_failure;
				}
			}
		}
	}


	/* query */
	php_mongo_runquery(cursor TSRMLS_CC);
	if (EG(exception)) {
		zval_ptr_dtor(&zcursor);
		RETURN_NULL();
	}

	/* Find return value */
	if (php_mongocursor_load_current_element(cursor TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&zcursor);
		RETURN_NULL();
	}

	if (php_mongo_handle_error(cursor TSRMLS_CC)) {
		zval_ptr_dtor(&zcursor);
		RETURN_NULL();
	}

	if (!php_mongocursor_is_valid(cursor)) {
		zval_ptr_dtor(&zcursor);
		RETURN_NULL();
	}
	RETVAL_ZVAL(cursor->current, 1, 0);

cleanup_on_failure:
	zend_objects_store_del_ref(zcursor TSRMLS_CC);
	zval_ptr_dtor(&zcursor);
}

/* {{{ proto array MongoCollection::findOne([array|object criteria [, array|object return_fields [, array options]]])
   Return the first document that matches $criteria and use $return_fields as
   the projection. NULL will be returned if no document matches. */
PHP_METHOD(MongoCollection, findOne)
{
	zval *query = NULL, *fields = NULL, *options = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zza", &query, &fields, &options) == FAILURE) {
		return;
	}

	MUST_BE_ARRAY_OR_OBJECT(1, query);
	MUST_BE_ARRAY_OR_OBJECT(2, fields);

	php_mongo_collection_findone(getThis(), query, fields, options, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto array MongoCollection::findAndModify(array query [, array update [, array fields [, array options]]])
   Atomically update and return a document */
PHP_METHOD(MongoCollection, findAndModify)
{
	HashTable *query = NULL, *update = NULL, *fields = NULL;
	zval *zquery = NULL, *zupdate = NULL, *zfields = NULL, *options = NULL;
	zval *cmd, *retval, **values;
	mongo_connection *used_connection;
	mongo_collection *c;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "H!|H!H!a!", &query, &update, &fields, &options) == FAILURE) {
		return;
	}

	c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);
	PHP_MONGO_GET_DB(c->parent);


	MAKE_STD_ZVAL(cmd);
	array_init(cmd);

	add_assoc_zval(cmd, "findandmodify", c->name);
	zval_add_ref(&c->name);

	if (query && zend_hash_num_elements(query) > 0) {
		MAKE_STD_ZVAL(zquery);
		array_init(zquery);
		zend_hash_copy(HASH_P(zquery), query, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		add_assoc_zval(cmd, "query", zquery);
	}

	if (update && zend_hash_num_elements(update) > 0) {
		MAKE_STD_ZVAL(zupdate);
		array_init(zupdate);
		zend_hash_copy(HASH_P(zupdate), update, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		add_assoc_zval(cmd, "update", zupdate);
	}

	if (fields && zend_hash_num_elements(fields) > 0) {
		MAKE_STD_ZVAL(zfields);
		array_init(zfields);
		zend_hash_copy(HASH_P(zfields), fields, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		add_assoc_zval(cmd, "fields", zfields);
	}

	if (options && zend_hash_num_elements(Z_ARRVAL_P(options)) > 0) {
		zval *temp;
		zend_hash_merge(HASH_P(cmd), HASH_P(options), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);
	}

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, &used_connection TSRMLS_CC);

	if (retval && php_mongo_trigger_error_on_command_failure(used_connection, retval TSRMLS_CC) == SUCCESS) {
		if (zend_hash_find(Z_ARRVAL_P(retval), "value", strlen("value") + 1, (void **)&values) == SUCCESS) {
			/* We may wind up with a NULL here if there simply aren't any results */
			if (Z_TYPE_PP(values) == IS_ARRAY) {
				array_init(return_value);
				zend_hash_copy(Z_ARRVAL_P(return_value), Z_ARRVAL_PP(values), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
			}
			/* If it's not an array, we should return NULL, which is the
			 * *default* return value and without "RETVAL_NULL" we simply do
			 * nothing */
		}
	} else {
		RETVAL_FALSE;
	}

	zval_ptr_dtor(&cmd);
	if (retval) {
		zval_ptr_dtor(&retval);
	}
}
/* }}} */

/* Takes OP_UPDATE flags (bit vector) and sets the correct update_args options */
static void mongo_apply_update_options_from_bits(php_mongo_write_update_args *update_options, int bits)
{
	update_options->upsert = bits & (1 << 0) ? 1 : 0;
	update_options->multi  = bits & (1 << 1) ? 1 : 0;
}

static void php_mongocollection_update(zval *this_ptr, mongo_collection *c, zval *criteria, zval *newobj, zval *z_write_options, zval *return_value TSRMLS_DC)
{
	int bit_opts = 0, is_acknowledged, supports_command, supports_opcode;
	mongo_connection *connection;
	mongoclient *link;

	if (z_write_options) {
		zval **upsert = 0, **multiple = 0;

		if (zend_hash_find(HASH_P(z_write_options), "upsert", strlen("upsert") + 1, (void**)&upsert) == SUCCESS) {
			convert_to_boolean_ex(upsert);
			bit_opts |= Z_BVAL_PP(upsert) << 0;
		}

		if (zend_hash_find(HASH_P(z_write_options), "multiple", strlen("multiple") + 1, (void**)&multiple) == SUCCESS) {
			convert_to_boolean_ex(multiple);
			bit_opts |= Z_BVAL_PP(multiple) << 1;
		}

		Z_ADDREF_P(z_write_options);
	} else {
		MAKE_STD_ZVAL(z_write_options);
		array_init(z_write_options);
	}

	PHP_MONGO_GET_LINK(c->link);
	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		zval_ptr_dtor(&z_write_options);
		RETURN_FALSE;
	}

	is_acknowledged = is_gle_op(this_ptr, z_write_options, &link->servers->options, QUIET TSRMLS_CC);
	supports_command = php_mongo_api_connection_supports_feature(connection, PHP_MONGO_API_WRITE_API);
	supports_opcode = php_mongo_api_connection_supports_feature(connection, PHP_MONGO_API_RELEASE_2_4_AND_BEFORE);

	/* Allow unacknowledged writes to be routed through legacy opcodes for
	 * performance, unless the server no longer supports the legacy opcode.
	 * This applies to the insert(), remove(), save(), and update() methods. */
	if (supports_command && (is_acknowledged || !supports_opcode)) {
		php_mongo_write_options write_options = {-1, {-1}, -1, -1, -1, -1};
		php_mongo_write_update_args update_options = { NULL, NULL, -1, -1 };
		int retval;
		mongo_db *db;
		int socket_read_timeout = 0;

		PHP_MONGO_GET_COLLECTION(getThis());
		PHP_MONGO_GET_DB(c->parent);

		update_options.query  = criteria;
		update_options.update = newobj;

		mongo_apply_update_options_from_bits(&update_options, bit_opts);
		mongo_apply_implicit_write_options(&write_options, &link->servers->options, getThis() TSRMLS_CC);
		php_mongo_api_write_options_from_zval(&write_options, z_write_options TSRMLS_CC);
		socket_read_timeout = mongo_get_socket_read_timeout(&link->servers->options, z_write_options TSRMLS_CC);

		retval = mongo_collection_update_api(link->manager, connection, &link->servers->options, socket_read_timeout, &update_options, &write_options, Z_STRVAL_P(db->name), getThis(), return_value TSRMLS_CC);

		if (retval) {
			/* Adds random "err", "code", "errmsg" empty fields to be compatible with
			 * old-style return values */
			mongo_convert_write_api_return_to_legacy_retval(return_value, MONGODB_API_COMMAND_UPDATE, write_options.wtype == 1 ? write_options.write_concern.w : 1 TSRMLS_CC);
		}
		zval_ptr_dtor(&z_write_options);
	} else if (supports_opcode) {
		int retval = 1;
		mongo_buffer buf;

		CREATE_BUF(buf, INITIAL_BUF_SIZE);
		if (FAILURE == php_mongo_write_update(&buf, Z_STRVAL_P(c->ns), bit_opts, criteria, newobj, connection->max_bson_size, connection->max_message_size TSRMLS_CC)) {
			efree(buf.start);
			zval_ptr_dtor(&z_write_options);
			return;
		}

		mongo_log_stream_update(connection, c->ns, criteria, newobj, z_write_options, bit_opts TSRMLS_CC);

		/* retval == -1 means a GLE response was received, so send_message() has
		 * either set return_value or thrown an exception via do_gle_op(). */
		retval = send_message(this_ptr, connection, &buf, z_write_options, return_value TSRMLS_CC);
		if (retval != -1) {
			RETVAL_BOOL(retval);
		}

		efree(buf.start);
		zval_ptr_dtor(&z_write_options);
	} else {
		zend_throw_exception_ex(mongo_ce_Exception, 0 TSRMLS_CC, "Cannot determine how to update documents on the server");
	}
}

/* {{{ proto bool|array MongoCollection::update(array|object criteria, array|object $newobj [, array options])
   Update one or more documents matching $criteria with $newobj and return the
   database response if the write concern is >= 1. Otherwise, boolean true is
   returned. */
PHP_METHOD(MongoCollection, update)
{
	zval *criteria, *newobj, *options = 0;
	mongo_collection *c;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|a/", &criteria, &newobj, &options) == FAILURE) {
		return;
	}

	MUST_BE_ARRAY_OR_OBJECT(1, criteria);
	MUST_BE_ARRAY_OR_OBJECT(2, newobj);

	PHP_MONGO_GET_COLLECTION(getThis());
	php_mongocollection_update(this_ptr, c, criteria, newobj, options, return_value TSRMLS_CC);
}
/* }}} */

static void mongo_apply_delete_options_from_bits(php_mongo_write_delete_args *delete_options, int bits)
{
	delete_options->limit = bits & (1 << 0) ? 1 : 0;
}

static void php_mongocollection_remove(zval *this_ptr, mongo_collection *c, zval *criteria, zval *z_write_options, zval *return_value TSRMLS_DC)
{
	int bit_opts = 0, is_acknowledged, supports_command, supports_opcode;
	mongo_connection *connection;
	mongoclient *link;

	if (!criteria) {
		MAKE_STD_ZVAL(criteria);
		array_init(criteria);
	} else {
		zval_add_ref(&criteria);
	}

	if (z_write_options) {
		zval **just_one = NULL;

		if (zend_hash_find(HASH_P(z_write_options), "justOne", strlen("justOne") + 1, (void**)&just_one) == SUCCESS) {
			convert_to_boolean_ex(just_one);
			bit_opts = Z_BVAL_PP(just_one) << 0;
		}
		Z_ADDREF_P(z_write_options);
	} else {
		MAKE_STD_ZVAL(z_write_options);
		array_init(z_write_options);
	}

	PHP_MONGO_GET_LINK(c->link);
	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		zval_ptr_dtor(&z_write_options);
		zval_ptr_dtor(&criteria);
		RETURN_FALSE;
	}

	is_acknowledged = is_gle_op(this_ptr, z_write_options, &link->servers->options, QUIET TSRMLS_CC);
	supports_command = php_mongo_api_connection_supports_feature(connection, PHP_MONGO_API_WRITE_API);
	supports_opcode = php_mongo_api_connection_supports_feature(connection, PHP_MONGO_API_RELEASE_2_4_AND_BEFORE);

	/* Allow unacknowledged writes to be routed through legacy opcodes for
	 * performance, unless the server no longer supports the legacy opcode.
	 * This applies to the insert(), remove(), save(), and update() methods. */
	if (supports_command && (is_acknowledged || !supports_opcode)) {
		php_mongo_write_options write_options = {-1, {-1}, -1, -1, -1, -1};
		php_mongo_write_delete_args delete_options = { NULL, -1 };
		int retval;
		mongo_db *db;
		int socket_read_timeout = 0;

		PHP_MONGO_GET_COLLECTION(getThis());
		PHP_MONGO_GET_DB(c->parent);

		delete_options.query  = criteria;

		mongo_apply_delete_options_from_bits(&delete_options, bit_opts);
		mongo_apply_implicit_write_options(&write_options, &link->servers->options, getThis() TSRMLS_CC);
		php_mongo_api_write_options_from_zval(&write_options, z_write_options TSRMLS_CC);
		socket_read_timeout = mongo_get_socket_read_timeout(&link->servers->options, z_write_options TSRMLS_CC);

		retval = mongo_collection_delete_api(link->manager, connection, &link->servers->options, socket_read_timeout, &delete_options, &write_options, Z_STRVAL_P(db->name), getThis(), return_value TSRMLS_CC);

		if (retval) {
			/* Adds random "err", "code", "errmsg" empty fields to be compatible with
			 * old-style return values */
			mongo_convert_write_api_return_to_legacy_retval(return_value, MONGODB_API_COMMAND_DELETE, write_options.wtype == 1 ? write_options.write_concern.w : 1 TSRMLS_CC);
		}
		zval_ptr_dtor(&z_write_options);
		zval_ptr_dtor(&criteria);
	} else if (supports_opcode) {
		mongo_buffer buf;
		int retval = 1;

		CREATE_BUF(buf, INITIAL_BUF_SIZE);
		if (FAILURE == php_mongo_write_delete(&buf, Z_STRVAL_P(c->ns), bit_opts, criteria, connection->max_bson_size, connection->max_message_size TSRMLS_CC)) {
			efree(buf.start);
			zval_ptr_dtor(&criteria);
			zval_ptr_dtor(&z_write_options);
			return;
		}
		mongo_log_stream_delete(connection, c->ns, criteria, z_write_options, bit_opts TSRMLS_CC);

		/* retval == -1 means a GLE response was received, so send_message() has
		 * either set return_value or thrown an exception via do_gle_op(). */
		retval = send_message(this_ptr, connection, &buf, z_write_options, return_value TSRMLS_CC);
		if (retval != -1) {
			RETVAL_BOOL(retval);
		}

		efree(buf.start);
		zval_ptr_dtor(&criteria);
		zval_ptr_dtor(&z_write_options);
	} else {
		zend_throw_exception_ex(mongo_ce_Exception, 0 TSRMLS_CC, "Cannot determine how to update documents on the server");
	}
}

/* {{{ proto bool|array MongoCollection::remove([array|object criteria [array options]])
   Remove one or more documents matching $criteria */
PHP_METHOD(MongoCollection, remove)
{
	zval *criteria = 0, *options = 0;
	mongo_collection *c;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|za/", &criteria, &options) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(1, criteria);
	PHP_MONGO_GET_COLLECTION(getThis());

	php_mongocollection_remove(this_ptr, c, criteria, options, return_value TSRMLS_CC);
}
/* }}} */


static void mongo_collection_create_index_command(mongo_connection *connection, zval *z_collection, zval *keys, zval *options, zval *return_value TSRMLS_DC)
{
	zval *cmd, *indexes, *index_spec, *retval;
	zend_bool done_name = 0;
	mongo_db *db;
	mongo_collection *c;

	PHP_MONGO_GET_COLLECTION(z_collection);

	db = (mongo_db*)zend_object_store_get_object(c->parent TSRMLS_CC);

	/* set up data */
	MAKE_STD_ZVAL(cmd);
	array_init(cmd);

	add_assoc_zval(cmd, "createIndexes", c->name);
	zval_add_ref(&c->name);

	/* set up "indexes" wrapper */
	MAKE_STD_ZVAL(indexes);
	array_init(indexes);
	add_assoc_zval(cmd, "indexes", indexes);

	/* set up index fields */
	MAKE_STD_ZVAL(index_spec);
	array_init(index_spec);
	add_next_index_zval(indexes, index_spec);

	/* add index keys */
	if (IS_SCALAR_P(keys)) {
		zval *key_array;

		convert_to_string(keys);

		if (Z_STRLEN_P(keys) == 0) {
			zend_throw_exception_ex(mongo_ce_Exception, 22 TSRMLS_CC, "empty string passed as key field");
			zval_ptr_dtor(&cmd);
			return;
		}

		MAKE_STD_ZVAL(key_array);
		array_init(key_array);
		add_assoc_long(key_array, Z_STRVAL_P(keys), 1);
		add_assoc_zval(index_spec, "key", key_array);
	} else if (Z_TYPE_P(keys) == IS_ARRAY || Z_TYPE_P(keys) == IS_OBJECT) {
		if (HASH_OF(keys)->nNumOfElements == 0) {
			zend_throw_exception_ex(mongo_ce_Exception, 22 TSRMLS_CC, "index specification has no elements");
			zval_ptr_dtor(&cmd);
			return;
		}

		add_assoc_zval(index_spec, "key", keys);
		Z_ADDREF_P(keys);
	} else {
		zend_throw_exception_ex(mongo_ce_Exception, 22 TSRMLS_CC, "index specification has to be an array");
		zval_ptr_dtor(&cmd);
		return;
	}

	/* process options */
	if (options) {
		zval *temp, **name, **maxtimems;

		/* "maxTimeMS" belongs on top-level command document */
		if (zend_hash_find(HASH_P(options), "maxTimeMS", strlen("maxTimeMS") + 1, (void**)&maxtimems) == SUCCESS) {
			add_assoc_zval(cmd, "maxTimeMS", *maxtimems);
			Z_ADDREF_PP(maxtimems);
			zend_hash_del(HASH_P(options), "maxTimeMS", strlen("maxTimeMS") + 1);
		}

		/* Write concern options are not relevant to the createIndexes command */
		DELETE_OPTION_IF_EXISTS(options, "fsync");
		DELETE_OPTION_IF_EXISTS(options, "j");
		DELETE_OPTION_IF_EXISTS(options, "safe");
		DELETE_OPTION_IF_EXISTS(options, "w");
		DELETE_OPTION_IF_EXISTS(options, "wtimeout");
		DELETE_OPTION_IF_EXISTS(options, "wTimeoutMS");

		zend_hash_merge(HASH_P(index_spec), HASH_P(options), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);

		/* "socketTimeoutMS" and "timeout" are php_mongo_runcommand() options */
		DELETE_OPTION_IF_EXISTS(index_spec, "socketTimeoutMS");
		DELETE_OPTION_IF_EXISTS(index_spec, "timeout");

		if (zend_hash_find(HASH_P(options), "name", strlen("name") + 1, (void**)&name) == SUCCESS) {
			done_name = 1;
			add_assoc_zval(index_spec, "name", *name);
			Z_ADDREF_PP(name);
		}
	}

	/* make sure we set the name, based on the keys, if no name was part of options */
	if (!done_name) {
		char *key_str;
		int   key_str_len;

		key_str = to_index_string(keys, &key_str_len TSRMLS_CC);
		if (!key_str) {
			zval_ptr_dtor(&cmd);
			return;
		}

		add_assoc_stringl(index_spec, "name", key_str, key_str_len, 0);
	}

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, options, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);

	if (!retval) {
		return;
	}

	if (php_mongo_trigger_error_on_command_failure(connection, retval TSRMLS_CC) == SUCCESS) {
		RETVAL_ZVAL(retval, 0, 1);
	} else {
		zval_ptr_dtor(&retval);
	}
}
/* }}} */


static void mongo_collection_create_index_legacy(mongo_connection *connection, zval *z_collection, zval *keys, zval *z_options, zval *return_value TSRMLS_DC)
{
	zval *db, *system_indexes_collection, *document;
	zend_bool done_name = 0;
	mongo_collection *c;

	PHP_MONGO_GET_COLLECTION(z_collection);

	if (IS_SCALAR_P(keys)) {
		zval *key_array;

		convert_to_string(keys);

		if (Z_STRLEN_P(keys) == 0) {
			zend_throw_exception_ex(mongo_ce_Exception, 22 TSRMLS_CC, "empty string passed as key field");
			return;
		}

		MAKE_STD_ZVAL(key_array);
		array_init(key_array);
		add_assoc_long(key_array, Z_STRVAL_P(keys), 1);

		keys = key_array;
	} else if (Z_TYPE_P(keys) == IS_ARRAY || Z_TYPE_P(keys) == IS_OBJECT) {
		if (HASH_OF(keys)->nNumOfElements == 0) {
			zend_throw_exception_ex(mongo_ce_Exception, 22 TSRMLS_CC, "index specification has no elements");
			return;
		}
		zval_add_ref(&keys);
	} else {
		zend_throw_exception_ex(mongo_ce_Exception, 22 TSRMLS_CC, "index specification has to be an array");
	}

	/* get the system.indexes collection */
	db = c->parent;

	system_indexes_collection = php_mongo_db_selectcollection(db, "system.indexes", strlen("system.indexes") TSRMLS_CC);
	PHP_MONGO_CHECK_EXCEPTION2(&keys, &system_indexes_collection);

	/* set up document */
	MAKE_STD_ZVAL(document);
	array_init(document);

	/* ns */
	add_assoc_zval(document, "ns", c->ns);
	zval_add_ref(&c->ns);
	add_assoc_zval(document, "key", keys);
	zval_add_ref(&keys);

	if (z_options) {
		zval *temp, **gle_pp, **fsync_pp, **timeout_pp, **name;

		zend_hash_merge(HASH_P(document), HASH_P(z_options), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);

		if (zend_hash_find(HASH_P(z_options), "safe", strlen("safe") + 1, (void**)&gle_pp) == SUCCESS) {
			zend_hash_del(HASH_P(document), "safe", strlen("safe") + 1);
		}
		if (zend_hash_find(HASH_P(z_options), "w", strlen("w") + 1, (void**)&gle_pp) == SUCCESS) {
			zend_hash_del(HASH_P(document), "w", strlen("w") + 1);
		}
		if (zend_hash_find(HASH_P(z_options), "fsync", strlen("fsync") + 1, (void**)&fsync_pp) == SUCCESS) {
			zend_hash_del(HASH_P(document), "fsync", strlen("fsync") + 1);
		}
		if (zend_hash_find(HASH_P(z_options), "timeout", strlen("timeout") + 1, (void**)&timeout_pp) == SUCCESS) {
			zend_hash_del(HASH_P(document), "timeout", strlen("timeout") + 1);
		}

		if (zend_hash_find(HASH_P(z_options), "name", strlen("name") + 1, (void**)&name) == SUCCESS) {
			done_name = 1;
		}
		zval_add_ref(&z_options);
	} else {
		zval *opts;

		MAKE_STD_ZVAL(opts);
		array_init(opts);
		z_options = opts;
	}

	if (!done_name) {
		char *key_str;
		int   key_str_len;

		key_str = to_index_string(keys, &key_str_len TSRMLS_CC);
		if (!key_str) {
			zval_ptr_dtor(&document);
			zval_ptr_dtor(&z_options);
			return;
		}

		add_assoc_stringl(document, "name", key_str, key_str_len, 0);
	}

	php_mongo_collection_insert(system_indexes_collection, document, z_options, return_value TSRMLS_CC);

	/* Check for whether an exception was thrown. In the special case where
	 * there is an index-adding problem, we need to change the exception to a
	 * different one */
	if (EG(exception)) {
		long code = 0;
		char *message;

		code = Z_LVAL_P(zend_read_property(mongo_ce_Exception, EG(exception), "code", strlen("code"), NOISY TSRMLS_CC));
		if (code == 10098 || code == 16734) {
			message = estrdup(Z_STRVAL_P(zend_read_property(mongo_ce_Exception, EG(exception), "message", strlen("message"), NOISY TSRMLS_CC)));
			zend_clear_exception(TSRMLS_C);
			php_mongo_cursor_throw(mongo_ce_ResultException, NULL, 67 TSRMLS_CC, "%s", message);
			efree(message);
		}
	}

	zval_ptr_dtor(&z_options);
	zval_ptr_dtor(&document);
	zval_ptr_dtor(&system_indexes_collection);
	zval_ptr_dtor(&keys);
}

/* {{{ proto bool MongoCollection::createIndex(array keys [, array options])
   Create the $keys index if it does not already exist */
PHP_METHOD(MongoCollection, createIndex)
{
	zval *keys, *options = NULL;
	mongo_connection *connection;
	mongo_collection *c;
	mongoclient *link;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|a", &keys, &options) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());

	PHP_MONGO_GET_LINK(c->link);
	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	if (php_mongo_api_connection_min_server_version(connection, 2, 5, 5)) {
		mongo_collection_create_index_command(connection, getThis(), keys, options, return_value TSRMLS_CC);
	} else {
		mongo_collection_create_index_legacy(connection, getThis(), keys, options, return_value TSRMLS_CC);
	}

	PHP_MONGO_GET_COLLECTION(getThis());
}
/* }}} */

/* {{{ proto bool MongoCollection::ensureIndex(mixed keys [, array options])
   Create the $keys index if it does not already exist */
PHP_METHOD(MongoCollection, ensureIndex)
{
	zval *keys, *options = NULL;
	mongo_connection *connection;
	mongo_collection *c;
	mongoclient *link;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &keys, &options) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());

	PHP_MONGO_GET_LINK(c->link);
	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	if (php_mongo_api_connection_min_server_version(connection, 2, 5, 5)) {
		mongo_collection_create_index_command(connection, getThis(), keys, options, return_value TSRMLS_CC);
	} else {
		mongo_collection_create_index_legacy(connection, getThis(), keys, options, return_value TSRMLS_CC);
	}

	PHP_MONGO_GET_COLLECTION(getThis());
}
/* }}} */

/* {{{ proto array MongoCollection::deleteIndex(mixed keys)
   Remove the $keys index */
PHP_METHOD(MongoCollection, deleteIndex)
{
	zval *keys, *cmd, *retval;
	char *key_str;
	int   key_str_len;
	mongo_collection *c;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &keys) == FAILURE) {
		return;
	}

	key_str = to_index_string(keys, &key_str_len TSRMLS_CC);
	if (!key_str) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_zval(cmd, "dropIndexes", c->name);
	zval_add_ref(&c->name);
	add_assoc_string(cmd, "index", key_str, 1);

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);
	efree(key_str);

	if (retval) {
		RETVAL_ZVAL(retval, 0, 1);
	}
}
/* }}} */

/* {{{ proto array MongoCollection::deleteIndex()
   Removes all indexes for this collection */
PHP_METHOD(MongoCollection, deleteIndexes)
{
	zval *cmd, *retval;
	mongo_collection *c;
	mongo_db *db;

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);

	add_assoc_string(cmd, "dropIndexes", Z_STRVAL_P(c->name), 1);
	add_assoc_string(cmd, "index", "*", 1);

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);

	zval_ptr_dtor(&cmd);

	if (retval) {
		RETURN_ZVAL(retval, 0, 1);
	}
}
/* }}} */

void mongo_collection_list_indexes_command(zval *this_ptr, zval *return_value TSRMLS_DC)
{
	zval *z_cmd, *list, **indexes;
	mongo_collection *c;
	mongo_db *db;
	mongo_connection *connection;
	zval *cursor_env, *retval;
	zval *tmp_iterator, *exception;
	mongo_cursor *cmd_cursor;

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(z_cmd);
	array_init(z_cmd);
	add_assoc_string(z_cmd, "listIndexes", Z_STRVAL_P(c->name), 1);

	/* Note: when this function is enhanced to support command options,
	 * php_mongo_validate_cursor_on_command() should be used here. */

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), z_cmd, NULL, 0, &connection TSRMLS_CC);
	
	zval_ptr_dtor(&z_cmd);
	
	if (!retval) {
		return;
	}

	if (php_mongo_trigger_error_on_command_failure(connection, retval TSRMLS_CC) == FAILURE) {
		/* The command method throws an error if the collection does not exist.
		 * If we hit that condition, we just clear the exception (and return an
		 * empty list) */
		if (EG(exception)) {
			zval *code;

			code = zend_read_property(mongo_ce_ResultException, EG(exception), "code", strlen("code"), 0 TSRMLS_CC);
			if (Z_TYPE_P(code) == IS_LONG && Z_LVAL_P(code) == PHP_MONGO_COLLECTION_DOES_NOT_EXIST) {
				zend_clear_exception(TSRMLS_C);
				zval_ptr_dtor(&retval);
				MAKE_STD_ZVAL(retval);
				array_init(retval);
			}
		}
		RETURN_ZVAL(retval, 0, 1);
	}
	
	/* list to return */
	MAKE_STD_ZVAL(list);
	array_init(list);

	/* Handle inline command response from server >= 2.7.5 and < 2.8.0-RC3. */
	if (zend_hash_find(Z_ARRVAL_P(retval), "indexes", strlen("indexes") + 1, (void **)&indexes) == SUCCESS) {
		HashPosition pointer;
		zval **index_doc;

		for (
				zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(indexes), &pointer);
				zend_hash_get_current_data_ex(Z_ARRVAL_PP(indexes), (void**)&index_doc, &pointer) == SUCCESS;
				zend_hash_move_forward_ex(Z_ARRVAL_PP(indexes), &pointer)
		) {
			Z_ADDREF_PP(index_doc);
			add_next_index_zval(list, *index_doc);
		}

		zval_ptr_dtor(&retval);

		RETURN_ZVAL(list, 0, 1);
	}

	MAKE_STD_ZVAL(tmp_iterator);
	php_mongo_commandcursor_instantiate(tmp_iterator TSRMLS_CC);
	cmd_cursor = (mongo_command_cursor*) zend_object_store_get_object(tmp_iterator TSRMLS_CC);

	/* We need to parse the initial result, and find the "cursor" element in the result */
	if (php_mongo_get_cursor_info_envelope(retval, &cursor_env TSRMLS_CC) == FAILURE) {
		exception = php_mongo_cursor_throw(mongo_ce_CursorException, cmd_cursor->connection, 30 TSRMLS_CC, "the command cursor did not return a correctly structured response");
		zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), retval TSRMLS_CC);
		zval_ptr_dtor(&tmp_iterator);
		return;
	}

	php_mongo_command_cursor_init_from_document(db->link, cmd_cursor, connection->hash, cursor_env TSRMLS_CC);

	/* TODO: Refactor to use MongoCommandCursor::rewind() once it is extracted
	 * into its own C function (see PHP-1362) */
	php_mongocommandcursor_fetch_batch_if_first_is_empty(cmd_cursor TSRMLS_CC);
	cmd_cursor->started_iterating = 1;

	for (
		php_mongocommandcursor_load_current_element(cmd_cursor TSRMLS_CC);
		php_mongocommandcursor_is_valid(cmd_cursor);
		php_mongocommandcursor_advance(cmd_cursor TSRMLS_CC)
	) {
		zval **index_doc = &cmd_cursor->current;

		Z_ADDREF_PP(index_doc);
		add_next_index_zval(list, *index_doc);
	}

	zval_ptr_dtor(&retval);
	zval_ptr_dtor(&tmp_iterator);

	RETURN_ZVAL(list, 0, 1);
}

void mongo_collection_list_indexes_legacy(zval *this_ptr, zval *return_value TSRMLS_DC)
{
	zval *z_system_collection, *query, *z_cursor, *list;
	mongo_collection *c, *system_collection;
	mongo_cursor *cursor;
	PHP_MONGO_GET_COLLECTION(getThis());

	/* select db.system.namespaces collection */
	z_system_collection = php_mongo_db_selectcollection(c->parent, "system.indexes", strlen("system.indexes") TSRMLS_CC);
	if (!z_system_collection) {
		/* An exception is set in this case */
		return;
	}

	MAKE_STD_ZVAL(query);
	array_init(query);
	add_assoc_string(query, "ns", Z_STRVAL_P(c->ns), 1);

	/* list to return */
	MAKE_STD_ZVAL(list);
	array_init(list);

	/* do find */
	MAKE_STD_ZVAL(z_cursor);
	object_init_ex(z_cursor, mongo_ce_Cursor);
	cursor = (mongo_cursor*)zend_object_store_get_object(z_cursor TSRMLS_CC);
	system_collection = (mongo_collection*)zend_object_store_get_object(z_system_collection TSRMLS_CC);

	php_mongo_collection_find(cursor, system_collection, query, NULL TSRMLS_CC);

	php_mongo_runquery(cursor TSRMLS_CC);
	zval_ptr_dtor(&query);

	if (EG(exception)) {
		zval_ptr_dtor(&z_cursor);
		zval_ptr_dtor(&z_system_collection);
		RETURN_ZVAL(list, 0, 1);
	}

	/* populate list */
	php_mongocursor_load_current_element(cursor TSRMLS_CC);

	if (php_mongo_handle_error(cursor TSRMLS_CC)) {
		zval_ptr_dtor(&z_cursor);
		zval_ptr_dtor(&z_system_collection);
		RETURN_ZVAL(list, 0, 1);
	}

	while (php_mongocursor_is_valid(cursor)) {
		Z_ADDREF_P(cursor->current);
		add_next_index_zval(list, cursor->current);

		php_mongocursor_advance(cursor TSRMLS_CC);
	}

	zval_ptr_dtor(&z_cursor);
	zval_ptr_dtor(&z_system_collection);

	RETURN_ZVAL(list, 0, 1);
}

/* {{{ proto MongoCollection::getIndexInfo()
   Get all indexes for this collection */
PHP_METHOD(MongoCollection, getIndexInfo)
{
	mongo_connection *connection;
	mongo_collection *c;
	mongoclient *link;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_LINK(c->link);

	if ((connection = php_mongo_collection_get_server(link, MONGO_CON_FLAG_WRITE TSRMLS_CC)) == NULL) {
		return;
	}

	if (php_mongo_api_connection_min_server_version(connection, 2, 7, 5)) {
		mongo_collection_list_indexes_command(getThis(), return_value TSRMLS_CC);
	} else {
		mongo_collection_list_indexes_legacy(getThis(), return_value TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto MongoCollection::count([array criteria [, array options])
   Count all documents matching $criteria. For legacy compatibility, "limit" and
   "skip" are accepted as the second and third arguments, respectively. */
PHP_METHOD(MongoCollection, count)
{
	long limit = 0, skip = 0;
	HashTable *query = 0, *options = 0;
	zval *response, *cmd, *zquery = 0;
	zval **n;
	mongo_connection *used_connection;
	mongo_collection *c;
	mongo_db *db;

	if (
		zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "|Hll", &query, &limit, &skip) == FAILURE &&
		zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|Hh", &query, &options) == FAILURE
	) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_string(cmd, "count", Z_STRVAL_P(c->name), 1);

	if (query && zend_hash_num_elements(query) > 0) {
		MAKE_STD_ZVAL(zquery);
		array_init(zquery);
		zend_hash_copy(HASH_P(zquery), query, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		add_assoc_zval(cmd, "query", zquery);
	}

	if (options) {
		zval *temp;
		zend_hash_merge(HASH_P(cmd), options, (copy_ctor_func_t) zval_add_ref, &temp, sizeof(zval*), 1);
	}

	if (limit) {
		add_assoc_long(cmd, "limit", limit);
	}

	if (skip) {
		add_assoc_long(cmd, "skip", skip);
	}

	response = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, &used_connection TSRMLS_CC);

	if (response) {
		if (php_mongo_trigger_error_on_command_failure(used_connection, response TSRMLS_CC) == SUCCESS) {
			if (zend_hash_find(HASH_P(response), ZEND_STRS("n"), (void**)&n) == SUCCESS) {
				convert_to_long(*n);
				RETVAL_ZVAL(*n, 1, 0);
			} else {
				php_mongo_cursor_throw(mongo_ce_ResultException, used_connection, 20 TSRMLS_CC, "Number of matched documents missing from count command response");
			}
		}

		zval_ptr_dtor(&response);
	}

	zval_ptr_dtor(&cmd);
}
/* }}} */

/* {{{ proto mixed MongoCollection::save(array|object document [, array options])
   Saves $document to this collection. An upsert will be used if the document's
   _id is set; otherwise, it will be inserted. Return the database response if
   the write concern is >= 1. Otherwise, boolean true is returned if the
   document is not empty. */
PHP_METHOD(MongoCollection, save)
{
	zval *document, *z_options = NULL;
	zval **id;
	mongo_collection *c;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a/", &document, &z_options) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(1, document);

	if (!z_options) {
		MAKE_STD_ZVAL(z_options);
		array_init(z_options);
	} else {
		Z_ADDREF_P(z_options);
	}

	if (zend_hash_find(HASH_P(document), "_id", 4, (void**)&id) == SUCCESS) {
		zval *criteria;

		MAKE_STD_ZVAL(criteria);
		array_init(criteria);
		add_assoc_zval(criteria, "_id", *id);
		zval_add_ref(id);

		add_assoc_bool(z_options, "upsert", 1);

		PHP_MONGO_GET_COLLECTION(getThis());
		php_mongocollection_update(this_ptr, c, criteria, document, z_options, return_value TSRMLS_CC);

		zval_ptr_dtor(&criteria);
		zval_ptr_dtor(&z_options);
		return;
	}

	php_mongo_collection_insert(this_ptr, document, z_options, return_value TSRMLS_CC);
	zval_ptr_dtor(&z_options);
}
/* }}} */

/* {{{ proto array MongoCollection::createDBRef(array dbref)
    Create a database reference object */
PHP_METHOD(MongoCollection, createDBRef)
{
	zval *obj;
	mongo_collection *c;
	mongo_db *db;
	zval *retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &obj) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_DB(c->parent);

	if (
		(obj = php_mongo_dbref_resolve_id(obj TSRMLS_CC)) &&
		(retval = php_mongo_dbref_create(obj, Z_STRVAL_P(c->name), NULL TSRMLS_CC))
	) {
		RETURN_ZVAL(retval, 0, 1);
	}

	RETURN_NULL();
}
/* }}} */

/* {{{ proto array MongoCollection::getDBRef(array dbref)
   Retrieves the document referenced by $dbref */
PHP_METHOD(MongoCollection, getDBRef)
{
	zval *ref;
	mongo_collection *c;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(1, ref);

	PHP_MONGO_GET_COLLECTION(getThis());
	php_mongo_dbref_get(c->parent, ref, return_value TSRMLS_CC);
}
/* }}} */

static void replace_dots(char *key, int key_len)
{
	int i;

	for (i = 0; i < key_len; i++) {
		if (key[i] == '.') {
			key[i] = '_';
		}
	}
}

static char *to_index_string(zval *zkeys, int *key_len TSRMLS_DC)
{
	smart_str str = { NULL, 0, 0 };

	switch (Z_TYPE_P(zkeys)) {
		case IS_ARRAY:
		case IS_OBJECT: {
			HashTable *hindex = HASH_P(zkeys);
			HashPosition pointer;
			zval **data;
			char *key;
			uint index_key_len, first = 1, key_type;
			ulong index;

			for (
				zend_hash_internal_pointer_reset_ex(hindex, &pointer);
				zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS;
				zend_hash_move_forward_ex(hindex, &pointer)
			) {
				if (!first) {
					smart_str_appendc(&str, '_');
				}
				first = 0;

				key_type = zend_hash_get_current_key_ex(hindex, &key, &index_key_len, &index, NO_DUP, &pointer);

				switch (key_type) {
					case HASH_KEY_IS_STRING:
						smart_str_appendl(&str, key, index_key_len - 1);
						break;

					case HASH_KEY_IS_LONG:
						smart_str_append_long(&str, (long) index);
						break;

					default:
						continue;
				}

				smart_str_appendc(&str, '_');

				switch (Z_TYPE_PP(data)) {
					case IS_STRING:
						smart_str_appendl(&str, Z_STRVAL_PP(data), Z_STRLEN_PP(data));
						break;

					case IS_BOOL:
						/* -1 cannot be expressed as a boolean, so the order is
						 * is ascending. Emit a notice if boolean false is used,
						 * since users may expect a descending order. */
						if (Z_BVAL_PP(data) == 0) {
							php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Boolean false ordering is ascending");
						}
						smart_str_append_long(&str, 1);
						break;

					case IS_LONG:
						smart_str_append_long(&str, Z_LVAL_PP(data) < 0 ? -1 : 1);
						break;

					case IS_DOUBLE:
						smart_str_append_long(&str, Z_DVAL_PP(data) < 0 ? -1 : 1);
						break;

					default:
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Key orderings must be scalar; %s given", zend_get_type_by_const(Z_TYPE_PP(data)));
						/* -1 cannot be expressed as a non-scalar, so the order
						 * is ascending. While the server will accept a null
						 * ordering, arrays and objects should be rejected. */
						smart_str_append_long(&str, 1);
				}
			}
		} break;

		case IS_STRING: {
			smart_str_appendl(&str, Z_STRVAL_P(zkeys), Z_STRLEN_P(zkeys));
			smart_str_appendl(&str, "_1", 2);
		} break;

		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "The key needs to be either a string or an array");
			return NULL;
	}

	smart_str_0(&str);

	replace_dots(str.c, str.len);

	if (key_len) {
		*key_len = str.len;
	}

	return str.c;
}

/* {{{ proto protected static string MongoCollection::toIndexString(array|string keys)
   Converts $keys to an identifying string for an index */
PHP_METHOD(MongoCollection, toIndexString)
{
	zval *zkeys;
	char *key_str;
	int   key_str_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zkeys) == FAILURE) {
		return;
	}

	key_str = to_index_string(zkeys, &key_str_len TSRMLS_CC);

	if (key_str) {
		RETVAL_STRING(key_str, 0);
	} else {
		return;
	}
}
/* }}} */

/* Returns 1 if the pipeline ends with an $out operator; returns 0 if not. */
static int php_mongodb_pipeline_ends_with_out(zval *pipeline TSRMLS_DC)
{
	zval **op;
	int ends_with_out = 0;

	zend_hash_internal_pointer_end(HASH_OF(pipeline));

	if (
		zend_hash_get_current_data(HASH_OF(pipeline), (void **)&op) == SUCCESS &&
		zend_hash_exists(HASH_OF(*op), "$out", 5)
	) {
		ends_with_out = 1;
	}

	zend_hash_internal_pointer_reset(HASH_OF(pipeline));

	return ends_with_out;
}

void php_mongodb_aggregate(zval *pipeline, zval *options, mongo_db *db, mongo_collection *collection, zval *return_value TSRMLS_DC)
{
	zval *cmd;
	zval *retval;
	mongo_connection *connection;
	mongo_read_preference *original_rp = NULL;

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);

	add_assoc_zval(cmd, "aggregate", collection->name);
	add_assoc_zval(cmd, "pipeline", pipeline);
	zval_add_ref(&collection->name);
	zval_add_ref(&pipeline);

	if (
		collection->read_pref.type != MONGO_RP_PRIMARY &&
		php_mongodb_pipeline_ends_with_out(pipeline TSRMLS_CC)
	) {
		mongo_manager_log(MonGlo(manager), MLOG_RS, MLOG_WARN, "Forcing aggregate with $out to run on primary");
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Forcing aggregate with $out to run on primary");
		/* Save the original read pref to ensure we replace it with a primary
		 * read pref that does not contain tag sets (PHP-1369) */
		original_rp = ecalloc(1, sizeof(mongo_read_preference));
		mongo_read_preference_copy(&collection->read_pref, original_rp);
		mongo_read_preference_dtor(&collection->read_pref);
		collection->read_pref.type = MONGO_RP_PRIMARY;
	}

	if (options) {
		zval *temp;
		zend_hash_merge(HASH_P(cmd), HASH_P(options), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);
	}

	retval = php_mongo_runcommand(collection->link, &collection->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, &connection TSRMLS_CC);
	if (retval && php_mongo_trigger_error_on_command_failure(connection, retval TSRMLS_CC) == SUCCESS) {
		RETVAL_ZVAL(retval, 0, 1);
	}

	/* Restore the collection's original read pref if we previously overrode it */
	if (original_rp != NULL) {
		mongo_read_preference_copy(original_rp, &collection->read_pref);
		mongo_read_preference_dtor(original_rp);
		efree(original_rp);
	}

	zval_ptr_dtor(&cmd);
}

/* {{{ proto array MongoCollection::aggregate(array pipeline [, array  options ])
       proto array MongoCollection::aggregate(array op [, array op [, array op]])
   Wrapper for aggregate command. The pipeline may be specified as a single
   array of operations or a variable number of operation arguments. Returns the
   database response for the command. Aggregation results will be stored in the
   "result" key of the response. */
PHP_METHOD(MongoCollection, aggregate)
{
	zval ***argv, *pipeline, *tmp, *options = NULL;
	int argc, i;
	mongo_collection *collection;
	mongo_db *db;

	collection = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(collection->ns, MongoCollection);
	PHP_MONGO_GET_DB(collection->parent);

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "a|a", &pipeline, &options) == SUCCESS) {
		if (php_mongo_is_numeric_array(pipeline TSRMLS_CC) == SUCCESS) {
			php_mongodb_aggregate(pipeline, options, db, collection, return_value TSRMLS_CC);
			return;
		}
		/* If its not numeric array then we have $pipe, $pipe, $pipe, $pipe, ... */
	}

	/* array, array, array, array, .... */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "+", &argv, &argc) == FAILURE) {
		return;
	}

	for (i = 0; i < argc; i++) {
		tmp = *argv[i];
		if (Z_TYPE_P(tmp) != IS_ARRAY) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Argument %d is not an array", i + 1);
			efree(argv);
			return;
		}
	}

	MAKE_STD_ZVAL(pipeline);
	array_init(pipeline);

	for (i = 0; i < argc; i++) {
		tmp = *argv[i];
		Z_ADDREF_P(tmp);
		if (zend_hash_next_index_insert(Z_ARRVAL_P(pipeline), &tmp, sizeof(zval*), NULL) == FAILURE) {
			Z_DELREF_P(tmp);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create pipeline array");
			efree(argv);
			RETURN_FALSE;
		}
	}
	php_mongodb_aggregate(pipeline, NULL, db, collection, return_value TSRMLS_CC);
	zval_ptr_dtor(&pipeline);
	efree(argv);
}
/* }}} */


static zval* create_aggregate_command_from_pipeline(char *collname, zval *pipeline, zval *options TSRMLS_DC)
{
	zval *command;

	MAKE_STD_ZVAL(command);
	array_init(command);

	/* Command entry */
	add_assoc_string(command, "aggregate", collname, 1);

	/* Pipeline */
	add_assoc_zval(command, "pipeline", pipeline);
	Z_ADDREF_P(pipeline);

	if (options) {
		zval *temp;
		zend_hash_merge(HASH_P(command), HASH_P(options), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);
	}

	/* Ensure that the command's cursor option is set and valid. If this fails,
	 * EG(exception) is set. */
	if (!php_mongo_enforce_cursor_on_command(command TSRMLS_CC)) {
		zval_ptr_dtor(&command);
		return NULL;
	}

	return command;
}

/* {{{ proto MongoCommandCursor MongoCollection::aggregateCursor(array pipeline [, array options ]])
   Returns a command cursor after running the specified aggregation pipeline. */
PHP_METHOD(MongoCollection, aggregateCursor)
{
	zval *pipeline = NULL, *options = NULL, *command = NULL;
	mongo_collection *c;
	mongo_cursor *cmd_cursor;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|a", &pipeline, &options) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());

	command = create_aggregate_command_from_pipeline(Z_STRVAL_P(c->name), pipeline, options TSRMLS_CC);
	if (!command) {
		return;
	}

	object_init_ex(return_value, mongo_ce_CommandCursor);

	cmd_cursor = (mongo_cursor*)zend_object_store_get_object(return_value TSRMLS_CC);
	mongo_command_cursor_init(cmd_cursor, Z_STRVAL_P(c->ns), c->link, command TSRMLS_CC);
	zval_ptr_dtor(&command);

	/* Add read preferences to cursor, overriding the one set on the link */
	mongo_read_preference_replace(&c->read_pref, &cmd_cursor->read_pref);

	if (
		cmd_cursor->read_pref.type != MONGO_RP_PRIMARY &&
		php_mongodb_pipeline_ends_with_out(pipeline TSRMLS_CC)
	) {
		mongo_manager_log(MonGlo(manager), MLOG_RS, MLOG_WARN, "Forcing aggregate with $out to run on primary");
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Forcing aggregate with $out to run on primary");
		/* Destroy the read pref to ensure tag sets are cleared (PHP-1369) */
		mongo_read_preference_dtor(&cmd_cursor->read_pref);
		cmd_cursor->read_pref.type = MONGO_RP_PRIMARY;
	}
}
/* }}} */


/* {{{ proto array MongoCollection::distinct(string key [, array query])
   Wrapper for distinct command. Returns a list of distinct values for the given
   key across a collection. An optional $query may be applied to filter the
   documents considered. */
PHP_METHOD(MongoCollection, distinct)
{
	char *key;
	int key_len;
	HashTable *query = NULL;
	zval *cmd, **values, *tmp, *zquery = NULL;
	mongo_collection *c;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|H!", &key, &key_len, &query) == FAILURE) {
		return;
	}

	c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);

	add_assoc_zval(cmd, "distinct", c->name);
	zval_add_ref(&c->name);
	add_assoc_stringl(cmd, "key", key, key_len, 1);

	if (query && zend_hash_num_elements(query) > 0) {
		MAKE_STD_ZVAL(zquery);
		array_init(zquery);
		zend_hash_copy(HASH_P(zquery), query, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		add_assoc_zval(cmd, "query", zquery);
	}

	tmp = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, NULL TSRMLS_CC);
	zval_ptr_dtor(&cmd);

	if (!tmp) {
		/* Exception thrown */
		return;
	}

	if (zend_hash_find(Z_ARRVAL_P(tmp), "values", strlen("values") + 1, (void **)&values) == SUCCESS) {
		array_init_size(return_value, zend_hash_num_elements(Z_ARRVAL_PP(values)));
		zend_hash_copy(Z_ARRVAL_P(return_value), Z_ARRVAL_PP(values), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
	} else {
		RETVAL_FALSE;
	}

	zval_ptr_dtor(&tmp);
}
/* }}} */

/* {{{ proto array MongoCollection::group(mixed keys, array initial, MongoCode reduce [, array options])
   Wrapper for group command. Returns the database response for the command.
   Aggregation results will be stored in the "retval" key of the response. */
PHP_METHOD(MongoCollection, group)
{
	zval *key, *initial, *options = 0, *group, *cmd, *reduce;
	zval *retval;
	zval **maxtimems = 0;
	mongo_connection *used_connection;
	mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
	mongo_db *db;

	MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);
	PHP_MONGO_GET_DB(c->parent);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzz|z", &key, &initial, &reduce, &options) == FAILURE) {
		return;
	}
	MUST_BE_ARRAY_OR_OBJECT(4, options);

	if (Z_TYPE_P(reduce) == IS_STRING) {
		zval *code;
		MAKE_STD_ZVAL(code);
		object_init_ex(code, mongo_ce_Code);

		php_mongocode_populate(code, Z_STRVAL_P(reduce), Z_STRLEN_P(reduce), NULL TSRMLS_CC);

		reduce = code;
	} else {
		zval_add_ref(&reduce);
	}

	MAKE_STD_ZVAL(group);
	array_init(group);
	add_assoc_zval(group, "ns", c->name);
	zval_add_ref(&c->name);
	add_assoc_zval(group, "$reduce", reduce);
	zval_add_ref(&reduce);

	if (Z_TYPE_P(key) == IS_OBJECT && Z_OBJCE_P(key) == mongo_ce_Code) {
		add_assoc_zval(group, "$keyf", key);
	} else if (IS_ARRAY_OR_OBJECT_P(key)) {
		add_assoc_zval(group, "key", key);
	} else {
		zval_ptr_dtor(&group);
		zval_ptr_dtor(&reduce);
		zend_throw_exception(mongo_ce_Exception, "MongoCollection::group takes an array, object, or MongoCode key", 0 TSRMLS_CC);
		return;
	}
	zval_add_ref(&key);

	/* options used to just be "condition" but now can be "condition" or
	 * "finalize" */
	if (options) {
		zval **condition = 0, **finalize = 0;

		/* new case */
		if (zend_hash_find(HASH_P(options), "condition", strlen("condition") + 1, (void**)&condition) == SUCCESS) {
			add_assoc_zval(group, "cond", *condition);
			zval_add_ref(condition);
		}
		if (zend_hash_find(HASH_P(options), "finalize", strlen("finalize") + 1, (void**)&finalize) == SUCCESS) {
			add_assoc_zval(group, "finalize", *finalize);
			zval_add_ref(finalize);
		}

		/* The maxTimeMS option needs to be added to the cmd object, not group.
		 * Check now, but add it to cmd later if the pointer is not null. */
		zend_hash_find(HASH_P(options), "maxTimeMS", strlen("maxTimeMS") + 1, (void**)&maxtimems);

		if (!condition && !finalize && !maxtimems) {
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "Implicitly passing condition as $options will be removed in the future");
			add_assoc_zval(group, "cond", options);
			zval_add_ref(&options);
		}
	}

	add_assoc_zval(group, "initial", initial);
	zval_add_ref(&initial);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_zval(cmd, "group", group);

	if (maxtimems) {
		add_assoc_zval(cmd, "maxTimeMS", *maxtimems);
		zval_add_ref(maxtimems);
	}

	retval = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, NULL, 0, &used_connection TSRMLS_CC);

	if (retval && php_mongo_trigger_error_on_command_failure(used_connection, retval TSRMLS_CC) == FAILURE) {
		RETVAL_FALSE;
	}

	zval_ptr_dtor(&cmd);
	zval_ptr_dtor(&reduce);

	if (retval) {
		RETURN_ZVAL(retval, 0, 1);
	}
}
/* }}} */

/* {{{ proto MongoCollection MongoCollection::__get(string name)
   Appends this collection name with a period and $name and returns a new
   MongoCollection for the combined string. This is used to allow for concisely
   selecting collections with dotted names. */
PHP_METHOD(MongoCollection, __get)
{
	/* This is a little trickier than the getters in Mongo and MongoDB... we
	 * need to combine the current collection name with the parameter passed
	 * in, get the parent db, then select the new collection from it. */
	zval *collection;
	char *full_name, *name;
	int full_name_len, name_len;
	mongo_collection *c;
	PHP_MONGO_GET_COLLECTION(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		return;
	}

	/* If this is "db", return the parent database.  This can't actually be a
	 * property of the obj because apache does weird things on object
	 * destruction that will cause the link to be destroyed twice. */
	if (strcmp(name, "db") == 0) {
		RETURN_ZVAL(c->parent, 1, 0);
	}

	full_name_len = spprintf(&full_name, 0, "%s.%s", Z_STRVAL_P(c->name), name);

	/* select this collection */
	collection = php_mongo_db_selectcollection(c->parent, full_name, full_name_len TSRMLS_CC);
	if (collection) {
		/* Only copy the zval into return_value if it worked. If collection is
		 * NULL here, an exception is set */
		RETVAL_ZVAL(collection, 0, 1);
	}
	efree(full_name);
}
/* }}} */

/* {{{ proto array MongoCollection::parallelCollectionScan(int num_cursors [, array options])
   Returns an array of a maximum of num_cursors MongoCommandCursor objects. */
PHP_METHOD(MongoCollection, parallelCollectionScan)
{
	zval *options = NULL;
	long num_cursors = 0;
	mongo_db *db;
	mongo_connection *connection;
	mongo_collection *c;
	zval *cmd, *document;
	zval **cursor_desc;
	mongo_cursor *cmd_cursor;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|a", &num_cursors, &options) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_COLLECTION(getThis());
	PHP_MONGO_GET_DB(c->parent);

	MAKE_STD_ZVAL(cmd);
	array_init(cmd);
	add_assoc_zval(cmd, "parallelCollectionScan", c->name);
	add_assoc_long(cmd, "numCursors", num_cursors);
	zval_add_ref(&c->name);

	document = php_mongo_runcommand(c->link, &c->read_pref, Z_STRVAL_P(db->name), Z_STRLEN_P(db->name), cmd, options, 0, &connection TSRMLS_CC);
	zval_ptr_dtor(&cmd);

	if (!document) {
		return;
	}
	if (php_mongo_trigger_error_on_command_failure(connection, document TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&document);
		return;
	}

	if (zend_hash_find(Z_ARRVAL_P(document), "cursors", sizeof("cursors"), (void **)&cursor_desc) == FAILURE || Z_TYPE_PP(cursor_desc) != IS_ARRAY) {
		zend_throw_exception_ex(mongo_ce_CursorException, 30 TSRMLS_CC, "Cursor command response does not have the expected structure");
		zval_ptr_dtor(&document);
		return;
	}

	{
		HashPosition pointer;
		zval **cursor_doc;

		array_init(return_value);
		for (
				zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(cursor_desc), &pointer);
				zend_hash_get_current_data_ex(Z_ARRVAL_PP(cursor_desc), (void**)&cursor_doc, &pointer) == SUCCESS;
				zend_hash_move_forward_ex(Z_ARRVAL_PP(cursor_desc), &pointer)
		) {
			zval *zcursor, **cursor_element;

			if (Z_TYPE_PP(cursor_doc) != IS_ARRAY) {
				continue;
			}
			if (zend_hash_find(Z_ARRVAL_PP(cursor_doc), "cursor", sizeof("cursor"), (void **)&cursor_element) == FAILURE || Z_TYPE_PP(cursor_element) != IS_ARRAY) {
				zend_throw_exception_ex(mongo_ce_Exception, 34 TSRMLS_CC, "Cursor structure is invalid");
				zval_ptr_dtor(&document);
				return;
			}

			MAKE_STD_ZVAL(zcursor);
			object_init_ex(zcursor, mongo_ce_CommandCursor);

			cmd_cursor = (mongo_cursor*)zend_object_store_get_object(zcursor TSRMLS_CC);
			php_mongo_command_cursor_init_from_document(c->link, cmd_cursor, connection->hash, *cursor_element TSRMLS_CC);

			add_next_index_zval(return_value, zcursor);
		}
	}

	zval_ptr_dtor(&document);
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(0, database, MongoDB, 0)
	ZEND_ARG_INFO(0, collection_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_distinct, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setSlaveOkay, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, slave_okay)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setReadPreference, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, read_preference)
	ZEND_ARG_ARRAY_INFO(0, tags, 0) /* Yes, this should be an array */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getWriteConcern, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setWriteConcern, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, w)
	ZEND_ARG_INFO(0, wtimeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_validate, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, validate)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_insert, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, array_of_fields_OR_object)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_batchInsert, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, documents, 0) /* Array of documents */
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_find, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, query)
	ZEND_ARG_INFO(0, fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_find_one, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, query)
	ZEND_ARG_INFO(0, fields)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_findandmodify, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, query)
	ZEND_ARG_INFO(0, update)
	ZEND_ARG_INFO(0, fields)
	ZEND_ARG_ARRAY_INFO(0, options, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_aggregatecursor, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, pipeline, 1)
	ZEND_ARG_ARRAY_INFO(0, options, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_update, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, old_array_of_fields_OR_object)
	ZEND_ARG_INFO(0, new_array_of_fields_OR_object)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_remove, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, array_of_fields_OR_object)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_createIndex, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, array_of_keys)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_ensureIndex, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, key_OR_array_of_keys)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_deleteIndex, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, string_OR_array_of_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_count, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, query_AS_array_of_fields_OR_object)
	ZEND_ARG_INFO(0, options_OR_limit)
	ZEND_ARG_INFO(0, skip)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_createDBRef, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, array_with_id_fields_OR_MongoID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getDBRef, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, reference)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_toIndexString, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, string_OR_array_of_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_group, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_INFO(0, keys_or_MongoCode)
	ZEND_ARG_INFO(0, initial_value)
	ZEND_ARG_INFO(0, array_OR_MongoCode)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_aggregate, 0, 0, 1)
	ZEND_ARG_INFO(0, pipeline)
	ZEND_ARG_INFO(0, op)
	ZEND_ARG_INFO(0, ...)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_parallelcollectionscan, 0, 0, 1)
	ZEND_ARG_INFO(0, num_cursors)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

static zend_function_entry MongoCollection_methods[] = {
	PHP_ME(MongoCollection, __construct, arginfo___construct, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, __toString, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, __get, arginfo___get, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, getName, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, getSlaveOkay, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoCollection, setSlaveOkay, arginfo_setSlaveOkay, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(MongoCollection, getReadPreference, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, setReadPreference, arginfo_setReadPreference, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, getWriteConcern, arginfo_getWriteConcern, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, setWriteConcern, arginfo_setWriteConcern, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, drop, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, validate, arginfo_validate, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, insert, arginfo_insert, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, batchInsert, arginfo_batchInsert, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, update, arginfo_update, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, remove, arginfo_remove, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, find, arginfo_find, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, findOne, arginfo_find_one, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, findAndModify, arginfo_findandmodify, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, createIndex, arginfo_createIndex, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, ensureIndex, arginfo_ensureIndex, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, deleteIndex, arginfo_deleteIndex, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, deleteIndexes, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, getIndexInfo, arginfo_no_parameters, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, count, arginfo_count, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, save, arginfo_insert, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, createDBRef, arginfo_createDBRef, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, getDBRef, arginfo_getDBRef, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, toIndexString, arginfo_toIndexString, ZEND_ACC_PROTECTED|ZEND_ACC_DEPRECATED|ZEND_ACC_STATIC)
	PHP_ME(MongoCollection, group, arginfo_group, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, distinct, arginfo_distinct, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, aggregate, arginfo_aggregate, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, aggregateCursor, arginfo_aggregatecursor, ZEND_ACC_PUBLIC)
	PHP_ME(MongoCollection, parallelCollectionScan, arginfo_parallelcollectionscan, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

static void php_mongo_collection_free(void *object TSRMLS_DC)
{
	mongo_collection *c = (mongo_collection*)object;

	if (c) {
		if (c->parent) {
			zval_ptr_dtor(&c->parent);
		}
		if (c->link) {
			zval_ptr_dtor(&c->link);
		}
		if (c->name) {
			zval_ptr_dtor(&c->name);
		}
		if (c->ns) {
			zval_ptr_dtor(&c->ns);
		}
		mongo_read_preference_dtor(&c->read_pref);
		zend_object_std_dtor(&c->std TSRMLS_CC);
		efree(c);
	}
}

/* {{{ php_mongo_collection_new
 */
zend_object_value php_mongo_collection_new(zend_class_entry *class_type TSRMLS_DC) {
	PHP_MONGO_OBJ_NEW(mongo_collection);
}
/* }}} */

void mongo_init_MongoCollection(TSRMLS_D) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoCollection", MongoCollection_methods);
	ce.create_object = php_mongo_collection_new;
	mongo_ce_Collection = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_class_constant_long(mongo_ce_Collection, "ASCENDING", strlen("ASCENDING"), 1 TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_Collection, "DESCENDING", strlen("DESCENDING"), -1 TSRMLS_CC);

	zend_declare_property_long(mongo_ce_Collection, "w", strlen("w"), 1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_long(mongo_ce_Collection, "wtimeout", strlen("wtimeout"), PHP_MONGO_DEFAULT_WTIMEOUT, ZEND_ACC_PUBLIC TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
