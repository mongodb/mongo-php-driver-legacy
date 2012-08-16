/**
 *  Copyright 2009-2011 10gen, Inc.
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
#include <ext/standard/md5.h>
#include "ext/standard/php_smart_str.h"

#include "php_mongo.h"
#include "util/pool.h"

#include "db.h"
#include "collection.h"
#include "cursor.h"
#include "gridfs.h"
#include "mongo_types.h"
#include "mcon/manager.h"

#ifndef zend_parse_parameters_none
#define zend_parse_parameters_none()    \
        zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")
#endif

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_Collection,
  *mongo_ce_Cursor,
  *mongo_ce_GridFS,
  *mongo_ce_Id,
  *mongo_ce_Code,
  *mongo_ce_Exception,
  *mongo_ce_CursorException,
  *mongo_ce_ConnectionException;

extern int le_pconnection,
  le_connection;

extern zend_object_handlers mongo_default_handlers;

zend_class_entry *mongo_ce_DB = NULL;

static void clear_exception(zval* return_value TSRMLS_DC);

void php_mongo_connection_force_primary(mongo_cursor *cursor, mongo_link *link)
{
	int   old_rp;
	char *error_message = NULL;

	if (link->servers->read_pref.type != MONGO_RP_PRIMARY) {
		cursor->connection = mongo_get_read_write_connection(link->manager, link->servers, 1, (char**) &error_message);
		if (!cursor->connection && error_message) {
			zend_throw_exception(mongo_ce_ConnectionException, error_message, 72 TSRMLS_CC);
			return;
		}
	}
}

/* {{{ MongoDB::__construct
 */
PHP_METHOD(MongoDB, __construct) {
  zval *zlink;
  char *name;
  int name_len;
  mongo_db *db;
  mongo_link *link;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &zlink, mongo_ce_Mongo, &name, &name_len) == FAILURE) {
    zval *object = getThis();
    ZVAL_NULL(object);
    return;
  }

  if (0 == name_len ||
      0 != strchr(name, ' ') || 0 != strchr(name, '.') || 0 != strchr(name, '\\') ||
      0 != strchr(name, '/') || 0 != strchr(name, '$')) {
#if ZEND_MODULE_API_NO >= 20060613
    zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "MongoDB::__construct(): invalid name %s", name);
#else
    zend_throw_exception_ex(zend_exception_get_default(), 0 TSRMLS_CC, "MongoDB::__construct(): invalid name %s", name);
#endif /* ZEND_MODULE_API_NO >= 20060613 */
    return;
  }

  db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);

  db->link = zlink;
  zval_add_ref(&db->link);

  PHP_MONGO_GET_LINK(zlink);
	mongo_read_preference_copy(&link->servers->read_pref, &db->read_pref);

  MAKE_STD_ZVAL(db->name);
  ZVAL_STRING(db->name, name, 1);
}
/* }}} */

PHP_METHOD(MongoDB, __toString) {
  mongo_db *db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED_STRING(db->name, MongoDB);
  RETURN_ZVAL(db->name, 1, 0);
}

PHP_METHOD(MongoDB, selectCollection) {
  zval temp;
  zval *z_collection;
  char *collection;
  int collection_len;
  mongo_db *db;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &collection, &collection_len) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(z_collection);
  ZVAL_STRINGL(z_collection, collection, collection_len, 1);

  db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(db->name, MongoDB);

  object_init_ex(return_value, mongo_ce_Collection);

  MONGO_METHOD2(MongoCollection, __construct, &temp, return_value, getThis(), z_collection);

  zval_ptr_dtor(&z_collection);
}

PHP_METHOD(MongoDB, getGridFS) {
  zval temp;
  zval *arg1 = 0, *arg2 = 0;

  // arg2 is deprecated
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &arg1, &arg2) == FAILURE) {
    return;
  }
  if (arg2) {
      php_error_docref(NULL TSRMLS_CC, MONGO_E_DEPRECATED, "This argument doesn't do anything. Please stop sending it");
  }

  object_init_ex(return_value, mongo_ce_GridFS);

  if (!arg1) {
    MONGO_METHOD1(MongoGridFS, __construct, &temp, return_value, getThis());
  }
  else {
    MONGO_METHOD2(MongoGridFS, __construct, &temp, return_value, getThis(), arg1);
  }
}

PHP_METHOD(MongoDB, getSlaveOkay)
{
	mongo_db *db;
	PHP_MONGO_GET_DB(getThis());
	RETURN_BOOL(db->read_pref.type != MONGO_RP_PRIMARY);
}

PHP_METHOD(MongoDB, setSlaveOkay)
{
	zend_bool slave_okay = 1;
	mongo_db *db;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &slave_okay) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	RETVAL_BOOL(db->read_pref.type != MONGO_RP_PRIMARY);
	db->read_pref.type = slave_okay ? MONGO_RP_SECONDARY_PREFERRED : MONGO_RP_PRIMARY;
}


PHP_METHOD(MongoDB, getReadPreference)
{
	mongo_db *db;
	PHP_MONGO_GET_DB(getThis());

	array_init(return_value);
	add_assoc_long(return_value, "type", db->read_pref.type);
	add_assoc_string(return_value, "type_string", mongo_read_preference_type_to_name(db->read_pref.type), 1);
	php_mongo_add_tagsets(return_value, &db->read_pref);
}

/* {{{ MongoDB::setReadPreference(int read_preference [, array tags ])
 * Sets a read preference to be used for all read queries.*/
PHP_METHOD(MongoDB, setReadPreference)
{
	long read_preference;
	mongo_db *db;
	HashTable  *tags = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|H", &read_preference, &tags) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_DB(getThis());

	if (read_preference >= MONGO_RP_FIRST && read_preference <= MONGO_RP_LAST) { 
		db->read_pref.type = read_preference;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The value %ld is not valid as read preference type", read_preference);
		RETURN_FALSE;
	}
	if (tags) {
		if (read_preference == MONGO_RP_PRIMARY) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "You can't use read preference tags with a read preference of PRIMARY");
			RETURN_FALSE;
		}

		if (!php_mongo_use_tagsets(&db->read_pref, tags TSRMLS_CC)) {
			RETURN_FALSE;
		}
	}
	RETURN_TRUE;
}
/* }}} */

PHP_METHOD(MongoDB, getProfilingLevel) {
  zval l;
  Z_TYPE(l) = IS_LONG;
  Z_LVAL(l) = -1;

  MONGO_METHOD1(MongoDB, setProfilingLevel, return_value, getThis(), &l);
}

PHP_METHOD(MongoDB, setProfilingLevel) {
  long level;
  zval *data, *cmd_return;
  zval **ok;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &level) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "profile", level);

  MAKE_STD_ZVAL(cmd_return);
  MONGO_CMD(cmd_return, getThis());

  zval_ptr_dtor(&data);

  if (EG(exception)) {
    zval_ptr_dtor(&cmd_return);
    return;
  }

  if (zend_hash_find(HASH_P(cmd_return), "ok", 3, (void**)&ok) == SUCCESS &&
      ((Z_TYPE_PP(ok) == IS_BOOL && Z_BVAL_PP(ok)) || Z_DVAL_PP(ok) == 1)) {
    zend_hash_find(HASH_P(cmd_return), "was", 4, (void**)&ok);
    RETVAL_ZVAL(*ok, 1, 0);
  }
  else {
    RETVAL_NULL();
  }
  zval_ptr_dtor(&cmd_return);
}

PHP_METHOD(MongoDB, drop) {
  zval *data;


  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }


  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "dropDatabase", 1);

  MONGO_CMD(return_value, getThis());
  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoDB, repair) {
  zend_bool cloned=0, original=0;
  zval *data;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|bb", &cloned, &original) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "repairDatabase", 1);
  add_assoc_bool(data, "preserveClonedFilesOnFailure", cloned);
  add_assoc_bool(data, "backupOriginalFiles", original);

  MONGO_CMD(return_value, getThis());

  zval_ptr_dtor(&data);
}


PHP_METHOD(MongoDB, createCollection) {
  zval *collection, *data, *temp;
  zend_bool capped=0;
  long size=0, max=0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|bll", &collection, &capped, &size, &max) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  convert_to_string(collection);
  add_assoc_zval(data, "create", collection);
  zval_add_ref(&collection);

  if (size) {
    add_assoc_long(data, "size", size);
  }

  if (capped) {
    add_assoc_bool(data, "capped", 1);
    if (max) {
      add_assoc_long(data, "max", max);
    }
  }

  MAKE_STD_ZVAL(temp);
  MONGO_CMD(temp, getThis());
  zval_ptr_dtor(&temp);

  zval_ptr_dtor(&data);

  if (!EG(exception)) {
    // get the collection we just created
    MONGO_METHOD1(MongoDB, selectCollection, return_value, getThis(), collection);
  }
}

PHP_METHOD(MongoDB, dropCollection) {
  zval *collection;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &collection) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(collection) != IS_OBJECT ||
      Z_OBJCE_P(collection) != mongo_ce_Collection) {
    zval *temp;

    MAKE_STD_ZVAL(temp);
    MONGO_METHOD1(MongoDB, selectCollection, temp, getThis(), collection);
    collection = temp;
  }
  else {
    zval_add_ref(&collection);
  }

  MONGO_METHOD(MongoCollection, drop, return_value, collection);

  zval_ptr_dtor(&collection);
}

static void php_mongo_enumerate_collections(INTERNAL_FUNCTION_PARAMETERS, int full_collection)
{
	zend_bool system_col = 0;
	zval *nss, *collection, *cursor, *list, *next;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &system_col) == FAILURE) {
		return;
	}

  // select db.system.namespaces collection
  MAKE_STD_ZVAL(nss);
  ZVAL_STRING(nss, "system.namespaces", 1);

  MAKE_STD_ZVAL(collection);
  MONGO_METHOD1(MongoDB, selectCollection, collection, getThis(), nss);

  // list to return
  MAKE_STD_ZVAL(list);
  array_init(list);

  // do find
  MAKE_STD_ZVAL(cursor);
  MONGO_METHOD(MongoCollection, find, cursor, collection);

  // populate list
  MAKE_STD_ZVAL(next);
  MONGO_METHOD(MongoCursor, getNext, next, cursor);
  while (!IS_SCALAR_P(next)) {
    zval *c, *zname;
    zval **collection;
    char *name, *first_dot, *system;

    // check that the ns is valid and not an index (contains $)
    if (zend_hash_find(HASH_P(next), "name", 5, (void**)&collection) == FAILURE ||
        strchr(Z_STRVAL_PP(collection), '$')) {

      zval_ptr_dtor(&next);
      MAKE_STD_ZVAL(next);
      ZVAL_NULL(next);

      MONGO_METHOD(MongoCursor, getNext, next, cursor);
      continue;
    }

    first_dot = strchr(Z_STRVAL_PP(collection), '.');
    system = strstr(Z_STRVAL_PP(collection), ".system.");
    // check that this isn't a system ns
    if ((!system_col && (system && first_dot == system)) ||
      (name = strchr(Z_STRVAL_PP(collection), '.')) == 0) {

      zval_ptr_dtor(&next);
      MAKE_STD_ZVAL(next);
      ZVAL_NULL(next);

      MONGO_METHOD(MongoCursor, getNext, next, cursor);
      continue;
    }

    // take a substring after the first "."
    name++;

    // "foo." was allowed in earlier versions
    if (name == '\0') {
      zval_ptr_dtor(&next);
      MAKE_STD_ZVAL(next);
      ZVAL_NULL(next);

      MONGO_METHOD(MongoCursor, getNext, next, cursor);
      continue;
    }

	if (full_collection) {
		MAKE_STD_ZVAL(c);
		ZVAL_NULL(c);

		MAKE_STD_ZVAL(zname);
		ZVAL_NULL(zname);

		// name must be copied because it is a substring of
		// a string that will be garbage collected in a sec
		ZVAL_STRING(zname, name, 1);
		MONGO_METHOD1(MongoDB, selectCollection, c, getThis(), zname);

		add_next_index_zval(list, c);

		zval_ptr_dtor(&zname);
	} else {
		add_next_index_string(list, name, 1);
	}
    zval_ptr_dtor(&next);
    MAKE_STD_ZVAL(next);

    MONGO_METHOD(MongoCursor, getNext, next, cursor);
  }

  zval_ptr_dtor(&next);
  zval_ptr_dtor(&nss);
  zval_ptr_dtor(&cursor);
  zval_ptr_dtor(&collection);

  RETURN_ZVAL(list, 0, 1);
}

PHP_METHOD(MongoDB, listCollections)
{
	php_mongo_enumerate_collections(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

PHP_METHOD(MongoDB, getCollectionNames)
{
	php_mongo_enumerate_collections(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

PHP_METHOD(MongoDB, createDBRef) {
  zval *ns, *obj;
  zval **id;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &ns, &obj) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(obj) == IS_ARRAY ||
      Z_TYPE_P(obj) == IS_OBJECT) {
    if (zend_hash_find(HASH_P(obj), "_id", 4, (void**)&id) == SUCCESS) {
      MONGO_METHOD2(MongoDBRef, create, return_value, NULL, ns, *id);
      return;
    }
    else if (Z_TYPE_P(obj) == IS_ARRAY) {
      return;
    }
  }

  MONGO_METHOD2(MongoDBRef, create, return_value, NULL, ns, obj);
}

PHP_METHOD(MongoDB, getDBRef) {
  zval *ref;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
    return;
  }

  MONGO_METHOD2(MongoDBRef, get, return_value, NULL, getThis(), ref);
}

PHP_METHOD(MongoDB, execute) {
	zval *code = 0, *args = 0, *zdata;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &code, &args) == FAILURE) {
		return;
	}

	// turn the first argument into MongoCode
	if (Z_TYPE_P(code) != IS_OBJECT ||
		Z_OBJCE_P(code) != mongo_ce_Code) {
		if (Z_TYPE_P(code) == IS_STRING) {
			zval *obj;

			MAKE_STD_ZVAL(obj);
			object_init_ex(obj, mongo_ce_Code);
			MONGO_METHOD1(MongoCode, __construct, return_value, obj, code);
			code = obj;
		} else { /* This is broken code */
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "The argument is neither an object of MongoCode or a string");
			return;
		}
	} else {
		zval_add_ref(&code);
	}

	if (!args) {
		MAKE_STD_ZVAL(args);
		array_init(args);
	} else {
		zval_add_ref(&args);
	}

	// create { $eval : code, args : [] }
	MAKE_STD_ZVAL(zdata);
	array_init(zdata);
	add_assoc_zval(zdata, "$eval", code);
	add_assoc_zval(zdata, "args", args);

	MONGO_METHOD1(MongoDB, command, return_value, getThis(), zdata);

	zval_ptr_dtor(&zdata);
}

static char *get_cmd_ns(char *db, int db_len) {
  char *position;

  char *cmd_ns = (char*)emalloc(db_len + strlen("$cmd") + 2);
  position = cmd_ns;

  // db
  memcpy(position, db, db_len);
  position += db_len;

  // .
  *(position)++ = '.';

  // $cmd
  memcpy(position, "$cmd", strlen("$cmd"));
  position += strlen("$cmd");

  // \0
  *(position) = '\0';

  return cmd_ns;
}

PHP_METHOD(MongoDB, command) {
  zval limit, *temp, *cmd, *cursor, *ns, *options = 0;
  mongo_db *db;
  mongo_link *link;
  char *cmd_ns;
	zval slave_okay;
	mongo_cursor *cursor_tmp;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &cmd, &options) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(cmd)) {
    zend_error(E_WARNING, "MongoDB::command() expects parameter 1 to be an array or object");
    return;
  }

  PHP_MONGO_GET_DB(getThis());

  // create db.$cmd
  MAKE_STD_ZVAL(ns);
  cmd_ns = get_cmd_ns(Z_STRVAL_P(db->name), Z_STRLEN_P(db->name));
  ZVAL_STRING(ns, cmd_ns, 0);

  // create cursor
  MAKE_STD_ZVAL(cursor);
  object_init_ex(cursor, mongo_ce_Cursor);
  MAKE_STD_ZVAL(temp);
  ZVAL_NULL(temp);

  MONGO_METHOD3(MongoCursor, __construct, temp, cursor, db->link, ns, cmd);

  zval_ptr_dtor(&ns);
  zval_ptr_dtor(&temp);
  MAKE_STD_ZVAL(temp);
  ZVAL_NULL(temp);

  // limit
  Z_TYPE(limit) = IS_LONG;
  Z_LVAL(limit) = -1;
  MONGO_METHOD1(MongoCursor, limit, temp, cursor, &limit);

  zval_ptr_dtor(&temp);

  if (options) {
    zval **timeout;
    if (zend_hash_find(HASH_P(options), "timeout", strlen("timeout")+1, (void**)&timeout) == SUCCESS) {
      MAKE_STD_ZVAL(temp);
      ZVAL_NULL(temp);
      MONGO_METHOD1(MongoCursor, timeout, temp, cursor, *timeout);
      zval_ptr_dtor(&temp);
    }
  }

	/* Make sure commands aren't be sent to slaves */
	/* TODO: The read preferences spec has a list of commands that *can* be send
	 * to slave */
	/* This should be refactored alongside with the getLastError redirection in
	 * collection.c/append_getlasterror. The Cursor creation should be done through
	 * an init method otherwise a connection have to be requested twice. */
	PHP_MONGO_GET_LINK(db->link);
	cursor_tmp = (mongo_cursor*)zend_object_store_get_object(cursor TSRMLS_CC);
	mongo_manager_log(link->manager, MLOG_CON, MLOG_INFO, "forcing primary for command");
	php_mongo_connection_force_primary(cursor_tmp, link);


  // query
  MONGO_METHOD(MongoCursor, getNext, return_value, cursor);
  clear_exception(return_value TSRMLS_CC);

  zend_objects_store_del_ref(cursor TSRMLS_CC);
  zval_ptr_dtor(&cursor);
}

zval* mongo_db__create_fake_cursor(mongo_connection *connection, char *database, zval *cmd TSRMLS_DC)
{
  zval *cursor_zval;
  mongo_cursor *cursor;
	smart_str ns = { 0 };

  MAKE_STD_ZVAL(cursor_zval);
  object_init_ex(cursor_zval, mongo_ce_Cursor);

  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);

  cursor->query = cmd;
  zval_add_ref(&cmd);

	if (database) {
		smart_str_append(&ns, database);
		smart_str_appendl(&ns, ".$cmd", 5);
		smart_str_0(&ns);
		cursor->ns = ns.c;
	} else {
		cursor->ns = estrdup("admin.$cmd");
	}

  cursor->fields = 0;
  cursor->limit = -1;
  cursor->skip = 0;
  cursor->opts = 0;
  cursor->current = 0;
  cursor->timeout = 0;

  return cursor_zval;
}

zval* mongo_db_cmd(mongo_connection *connection, char *database, zval *cmd TSRMLS_DC)
{
  zval temp_ret, *response, *cursor_zval;
  mongo_cursor *cursor = 0;
  int exception = 0;

	/* Create a cursor */
	cursor_zval = mongo_db__create_fake_cursor(connection, database, cmd TSRMLS_CC);
	cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);
	cursor->connection = connection;

  // need to call this after setting cursor->link
  // reset checks that cursor->link != 0
  MONGO_METHOD(MongoCursor, reset, &temp_ret, cursor_zval);

  MAKE_STD_ZVAL(response);
  ZVAL_NULL(response);

  MONGO_METHOD(MongoCursor, getNext, response, cursor_zval);
  if (EG(exception)) {
    zend_clear_exception(TSRMLS_C);
    exception = 1;
  }

	cursor->connection = 0;
  zval_ptr_dtor(&cursor_zval);

  if (exception || IS_SCALAR_P(response)) {
	  zval_ptr_dtor(&response);
    return 0;
  }

  return response;
}


static void md5_hash(char *md5str, char *arg) {
  PHP_MD5_CTX context;
  unsigned char digest[16];

  md5str[0] = '\0';
  PHP_MD5Init(&context);
  PHP_MD5Update(&context, arg, strlen(arg));
  PHP_MD5Final(digest, &context);
  make_digest(md5str, digest);
}


PHP_METHOD(MongoDB, authenticate) {
  char *username, *password;
  int ulen, plen;
  zval *data, *result, **nonce;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &username, &ulen, &password, &plen) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "getnonce", 1);

  MAKE_STD_ZVAL(result);
  ZVAL_NULL(result);
  MONGO_CMD(result, getThis());

  zval_ptr_dtor(&data);

  if (EG(exception)) {
    zval_ptr_dtor(&result);
    RETURN_FALSE;
  }

  if (zend_hash_find(HASH_P(result), "nonce", strlen("nonce")+1, (void**)&nonce) == SUCCESS) {
    char *salt, *rash;
    char hash[33], digest[33];

    // create username:mongo:password hash
    spprintf(&salt, 0, "%s:mongo:%s", username, password);
    md5_hash(hash, salt);
    efree(salt);

    // create nonce|username|hash hash
    spprintf(&rash, 0, "%s%s%s", Z_STRVAL_PP(nonce), username, hash);
    md5_hash(digest, rash);
    efree(rash);

    // make actual authentication cmd
    MAKE_STD_ZVAL(data);
    array_init(data);

    add_assoc_long(data, "authenticate", 1);
    add_assoc_stringl(data, "user", username, ulen, 1);
    add_assoc_zval(data, "nonce", *nonce);
    zval_add_ref(nonce);
    add_assoc_string(data, "key", digest, 1);

    MONGO_CMD(return_value, getThis());

    zval_ptr_dtor(&data);
  }
  else {
    RETVAL_FALSE;
  }

  zval_ptr_dtor(&result);
}

static void clear_exception(zval* return_value TSRMLS_DC) {
  if (EG(exception)) {
    zval *e, *doc;

    e = EG(exception);
    doc = zend_read_property(mongo_ce_CursorException, e, "doc", strlen("doc"), QUIET TSRMLS_CC);
    if (doc && Z_TYPE_P(doc) == IS_ARRAY &&
        !zend_hash_exists(Z_ARRVAL_P(doc), "$err", strlen("$err")+1)) {
      RETVAL_ZVAL(doc, 1, 0);
      zend_clear_exception(TSRMLS_C);
    }
  }
}


static void run_err(char *cmd, zval *return_value, zval *db TSRMLS_DC) {
  zval *data;

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, cmd, 1);

  MONGO_CMD(return_value, db);
  clear_exception(return_value TSRMLS_CC);

  zval_ptr_dtor(&data);
}

/* {{{ MongoDB->lastError()
 */
PHP_METHOD(MongoDB, lastError) {
  run_err("getlasterror", return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ MongoDB->prevError()
 */
PHP_METHOD(MongoDB, prevError) {
  run_err("getpreverror", return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ MongoDB->resetError()
 */
PHP_METHOD(MongoDB, resetError) {
  run_err("reseterror", return_value, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ MongoDB->forceError()
 */
PHP_METHOD(MongoDB, forceError) {
  run_err("forceerror", return_value, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ MongoDB::__get
 */
PHP_METHOD(MongoDB, __get) {
  zval *name;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
    return;
  }

  // select this collection
  MONGO_METHOD1(MongoDB, selectCollection, return_value, getThis(), name);
}
/* }}} */

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(0, connection, Mongo, 0)
	ZEND_ARG_INFO(0, database_name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_getGridFS, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, prefix)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_setSlaveOkay, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, slave_okay)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_setReadPreference, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, read_preference)
	ZEND_ARG_ARRAY_INFO(0, tags, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_setProfilingLevel, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_repair, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, keep_cloned_files)
	ZEND_ARG_INFO(0, backup_original_files)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_selectCollection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, collection_name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_createCollection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, collection_name)
	ZEND_ARG_INFO(0, capped)
	ZEND_ARG_INFO(0, capped_size)
	ZEND_ARG_INFO(0, max_elements)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_dropCollection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, collection_name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_createDBRef, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, collection_name)
	ZEND_ARG_INFO(0, array_with_id_fields_OR_MongoID)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_getDBRef, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, reference_information, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_execute, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, javascript_code)
	ZEND_ARG_ARRAY_INFO(0, arguments, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_command, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, command, 0)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_authenticate, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, username)
	ZEND_ARG_INFO(0, password)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_systemCollections, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, includeSystemCollections)
ZEND_END_ARG_INFO()


static zend_function_entry MongoDB_methods[] = {
  PHP_ME(MongoDB, __construct, arginfo___construct, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, __toString, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, __get, arginfo___get, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getGridFS, arginfo_getGridFS, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getSlaveOkay, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(MongoDB, setSlaveOkay, arginfo_setSlaveOkay, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(MongoDB, getReadPreference, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, setReadPreference, arginfo_setReadPreference, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getProfilingLevel, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, setProfilingLevel, arginfo_setProfilingLevel, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, drop, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, repair, arginfo_repair, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, selectCollection, arginfo_selectCollection, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, createCollection, arginfo_createCollection, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, dropCollection, arginfo_dropCollection, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, listCollections, arginfo_systemCollections, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getCollectionNames, arginfo_systemCollections, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, createDBRef, arginfo_createDBRef, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getDBRef, arginfo_getDBRef, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, execute, arginfo_execute, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, command, arginfo_command, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, lastError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, prevError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(MongoDB, resetError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(MongoDB, forceError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(MongoDB, authenticate, arginfo_authenticate, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  { NULL, NULL, NULL }
};

static void php_mongo_db_free(void *object TSRMLS_DC) {
  mongo_db *db = (mongo_db*)object;

  if (db) {
    if (db->link) {
      zval_ptr_dtor(&db->link);
    }
    if (db->name) {
      zval_ptr_dtor(&db->name);
    }
	mongo_read_preference_dtor(&db->read_pref);
    zend_object_std_dtor(&db->std TSRMLS_CC);
    efree(db);
  }
}


/* {{{ mongo_mongo_db_new
 */
zend_object_value php_mongo_db_new(zend_class_entry *class_type TSRMLS_DC) {
  php_mongo_obj_new(mongo_db);
}
/* }}} */


void mongo_init_MongoDB(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoDB", MongoDB_methods);
  ce.create_object = php_mongo_db_new;
  mongo_ce_DB = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_OFF", strlen("PROFILING_OFF"), 0 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_SLOW", strlen("PROFILING_SLOW"), 1 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_ON", strlen("PROFILING_ON"), 2 TSRMLS_CC);

  zend_declare_property_long(mongo_ce_DB, "w", strlen("w"), 1, ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_DB, "wtimeout", strlen("wtimeout"), PHP_MONGO_DEFAULT_TIMEOUT, ZEND_ACC_PUBLIC TSRMLS_CC);
}
