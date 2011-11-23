//collection.c
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

#include "php_mongo.h"
#include "collection.h"
#include "cursor.h"
#include "bson.h"
#include "mongo_types.h"
#include "db.h"
#include "util/link.h"
#include "util/rs.h"
#include "util/server.h"
#include "util/io.h"

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_DB,
  *mongo_ce_Cursor,
  *mongo_ce_Code,
  *mongo_ce_Exception;

extern int le_pconnection,
  le_connection;

extern zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_Collection = NULL;

static mongo_server* get_server(mongo_collection *c TSRMLS_DC);
static int is_safe_op(zval *options TSRMLS_DC);
static int safe_op(mongo_server *server, zval *cursor_z, buffer *buf, zval *return_value TSRMLS_DC);
static zval* append_getlasterror(zval *coll, buffer *buf, zval *options TSRMLS_DC);
/*
 * arginfo needs to be set for __get because if PHP doesn't know it only takes
 * one arg, it will issue a warning.
 */
#if ZEND_MODULE_API_NO < 20090115
static
#endif
ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_batchInsert, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, array_of_documents)
	ZEND_ARG_INFO(0, array_of_options)
ZEND_END_ARG_INFO()

PHP_METHOD(MongoCollection, __construct) {
  zval *parent, *name, *zns, *w, *wtimeout;
  mongo_collection *c;
  mongo_db *db;
  char *ns, *name_str;
  int name_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &parent, mongo_ce_DB, &name_str, &name_len) == FAILURE) {
    return;
  }

  // check for empty collection name
  if (name_len == 0) {
#if ZEND_MODULE_API_NO >= 20060613
    zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "MongoDB::__construct(): invalid name %s", name_str);
#else
    zend_throw_exception_ex(zend_exception_get_default(), 0 TSRMLS_CC, "MongoDB::__construct(): invalid name %s", name_str);
#endif /* ZEND_MODULE_API_NO >= 20060613 */
    return;
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);

  PHP_MONGO_GET_DB(parent);

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
  c->slave_okay = db->slave_okay;

  w = zend_read_property(mongo_ce_DB, parent, "w", strlen("w"), NOISY TSRMLS_CC);
  zend_update_property_long(mongo_ce_Collection, getThis(), "w", strlen("w"), Z_LVAL_P(w) TSRMLS_CC);
  wtimeout = zend_read_property(mongo_ce_DB, parent, "wtimeout", strlen("wtimeout"), NOISY TSRMLS_CC);
  zend_update_property_long(mongo_ce_Collection, getThis(), "wtimeout", strlen("wtimeout"), Z_LVAL_P(wtimeout) TSRMLS_CC);
}

PHP_METHOD(MongoCollection, __toString) {
  mongo_collection *c;
  PHP_MONGO_GET_COLLECTION(getThis());
  RETURN_ZVAL(c->ns, 1, 0);
}

PHP_METHOD(MongoCollection, getName) {
  mongo_collection *c;
  PHP_MONGO_GET_COLLECTION(getThis());
  RETURN_ZVAL(c->name, 1, 0);
}

PHP_METHOD(MongoCollection, getSlaveOkay) {
  mongo_collection *c;
  PHP_MONGO_GET_COLLECTION(getThis());
  RETURN_BOOL(c->slave_okay);
}

PHP_METHOD(MongoCollection, setSlaveOkay) {
  zend_bool slave_okay = 1;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &slave_okay) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  RETVAL_BOOL(c->slave_okay);
  c->slave_okay = slave_okay;
}

PHP_METHOD(MongoCollection, drop) {
  zval *data;
  mongo_collection *c;

  PHP_MONGO_GET_COLLECTION(getThis());

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_zval(data, "drop", c->name);
  zval_add_ref(&c->name);

  MONGO_CMD(return_value, c->parent);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, validate) {
  zval *data;
  zend_bool scan_data = 0;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &scan_data) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "validate", Z_STRVAL_P(c->name), 1);
  add_assoc_bool(data, "scandata", scan_data);

  MONGO_CMD(return_value, c->parent);

  zval_ptr_dtor(&data);
}

/*
 * this should probably be split into two methods... right now appends the
 * getlasterror query to the buffer and alloc & inits the cursor zval.
 */
static zval* append_getlasterror(zval *coll, buffer *buf, zval *options TSRMLS_DC) {
  zval *cmd_ns_z, *cmd, *cursor_z, *temp, *timeout_p;
  char *cmd_ns, *safe_str = 0;
  mongo_cursor *cursor;
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(coll TSRMLS_CC);
  mongo_db *db = (mongo_db*)zend_object_store_get_object(c->parent TSRMLS_CC);
  int response, safe = 0, fsync = 0, timeout = -1;

  GET_OPTIONS;

  // get "db.$cmd" zval
  MAKE_STD_ZVAL(cmd_ns_z);
  spprintf(&cmd_ns, 0, "%s.$cmd", Z_STRVAL_P(db->name));
  ZVAL_STRING(cmd_ns_z, cmd_ns, 0);

  // get {"getlasterror" : 1} zval
  MAKE_STD_ZVAL(cmd);
  array_init(cmd);
  add_assoc_long(cmd, "getlasterror", 1);

  if (safe == 1) {
    zval *w = zend_read_property(mongo_ce_Collection, coll, "w", strlen("w"), NOISY TSRMLS_CC);
    safe = Z_LVAL_P(w);
  }

  if (safe_str || safe > 1) {
    zval *wtimeout;

    if (safe_str) {
      add_assoc_string(cmd, "w", safe_str, 1);
    }
    else {
      add_assoc_long(cmd, "w", safe);
    }

    wtimeout = zend_read_property(mongo_ce_Collection, coll, "wtimeout", strlen("wtimeout"), NOISY TSRMLS_CC);
    add_assoc_long(cmd, "wtimeout", Z_LVAL_P(wtimeout));
  }
  if (fsync) {
    add_assoc_bool(cmd, "fsync", 1);
  }

  // get cursor
  MAKE_STD_ZVAL(cursor_z);
  object_init_ex(cursor_z, mongo_ce_Cursor);

  MAKE_STD_ZVAL(temp);
  ZVAL_NULL(temp);
  MONGO_METHOD2(MongoCursor, __construct, temp, cursor_z, c->link, cmd_ns_z);
  zval_ptr_dtor(&temp);
  if (EG(exception)) {
    zval_ptr_dtor(&cmd_ns_z);
    return 0;
  }

  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_z TSRMLS_CC);

  cursor->limit = -1;
  cursor->timeout = timeout;
  zval_ptr_dtor(&cursor->query);
  // cmd is now part of cursor, so it shouldn't be dtored until cursor is
  cursor->query = cmd;

  // append the query
  response = php_mongo_write_query(buf, cursor TSRMLS_CC);
  zval_ptr_dtor(&cmd_ns_z);

  if (FAILURE == response) {
    return 0;
  }

  return cursor_z;
}

static mongo_server* get_server(mongo_collection *c TSRMLS_DC) {
  zval *errmsg;
  mongo_link *link;
  mongo_server *server;

  link = (mongo_link*)zend_object_store_get_object((c->link) TSRMLS_CC);
  if (!link) {
    zend_throw_exception(mongo_ce_Exception, "The MongoCollection object has not been correctly initialized by its constructor", 17 TSRMLS_CC);
    return 0;
  }

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  if ((server = mongo_util_link_get_socket(link, errmsg TSRMLS_CC)) == 0) {
    mongo_cursor_throw(0, 16 TSRMLS_CC, Z_STRVAL_P(errmsg));
    zval_ptr_dtor(&errmsg);
    return 0;
  }

  zval_ptr_dtor(&errmsg);
  return server;
}

static int is_safe_op(zval *options TSRMLS_DC) {
  zval **safe_pp = 0, **fsync_pp = 0;

  return options &&
    ((zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_pp) == SUCCESS &&
      (Z_TYPE_PP(safe_pp) == IS_STRING ||
       ((Z_TYPE_PP(safe_pp) == IS_LONG || Z_TYPE_PP(safe_pp) == IS_BOOL) && Z_LVAL_PP(safe_pp) >= 1))) ||
     (zend_hash_find(HASH_P(options), "fsync", strlen("fsync")+1, (void**)&fsync_pp) == SUCCESS &&
      Z_BVAL_PP(fsync_pp) == 1));
}

static int safe_op(mongo_server *server, zval *cursor_z, buffer *buf, zval *return_value TSRMLS_DC) {
  zval *errmsg, **err;
  mongo_cursor *cursor;

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_z TSRMLS_CC);

  cursor->server = server;

  if (FAILURE == mongo_say(server, buf, errmsg TSRMLS_CC)) {
    mongo_util_link_failed(cursor->link, server TSRMLS_CC);
    mongo_cursor_throw(server, 16 TSRMLS_CC, Z_STRVAL_P(errmsg));

    zval_ptr_dtor(&errmsg);
    cursor->link = 0;
    zval_ptr_dtor(&cursor_z);
    return FAILURE;
  }

  // get reply
  if (FAILURE == php_mongo_get_reply(cursor, errmsg TSRMLS_CC)) {
    mongo_util_link_failed(cursor->link, server TSRMLS_CC);

    zval_ptr_dtor(&errmsg);
    cursor->link = 0;
    zval_ptr_dtor(&cursor_z);
    return FAILURE;
  }
  zval_ptr_dtor(&errmsg);

  cursor->started_iterating = 1;

  MONGO_METHOD(MongoCursor, getNext, return_value, cursor_z);

  if (EG(exception) ||
      (Z_TYPE_P(return_value) ==IS_BOOL && Z_BVAL_P(return_value) == 0)) {
    cursor->link = 0;
    zval_ptr_dtor(&cursor_z);
    return FAILURE;
  }
  // w timeout
  else if (zend_hash_find(Z_ARRVAL_P(return_value), "errmsg", strlen("errmsg")+1, (void**)&err) == SUCCESS &&
      Z_TYPE_PP(err) == IS_STRING) {
    zval **code;
    int status = zend_hash_find(Z_ARRVAL_P(return_value), "n", strlen("n")+1, (void**)&code);

    mongo_cursor_throw(cursor->server, (status == SUCCESS ? Z_LVAL_PP(code) : 0) TSRMLS_CC, Z_STRVAL_PP(err));

    cursor->link = 0;
    zval_ptr_dtor(&cursor_z);
    return FAILURE;
  }


  cursor->link = 0;
  zval_ptr_dtor(&cursor_z);
  return SUCCESS;
}


PHP_METHOD(MongoCollection, insert) {
  zval *a, *options = 0, *errmsg = 0;
  mongo_collection *c;
  mongo_server *server;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &a, &options) == FAILURE) {
    return;
  }

  if (IS_SCALAR_P(a)) {
    zend_error(E_WARNING, "MongoCollection::insert() expects parameter 1 to be an array or object");
    return;
  }

  // old boolean options
  if (options && !IS_SCALAR_P(options)) {
    zval_add_ref(&options);
  }
  else {
    zval *opts;
    MAKE_STD_ZVAL(opts);
    array_init(opts);

    if (options && IS_SCALAR_P(options)) {
      int safe = Z_BVAL_P(options);
      add_assoc_bool(opts, "safe", safe);
    }

    options = opts;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  if ((server = get_server(c TSRMLS_CC)) == 0) {
    RETURN_FALSE;
  }

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (FAILURE == php_mongo_write_insert(&buf, Z_STRVAL_P(c->ns), a,
                                        mongo_util_server_get_bson_size(server TSRMLS_CC) TSRMLS_CC)) {
    efree(buf.start);
    zval_ptr_dtor(&options);
    RETURN_FALSE;
  }

  SEND_MSG;

  efree(buf.start);
  zval_ptr_dtor(&options);
}

PHP_METHOD(MongoCollection, batchInsert) {
  zval *docs, *options = NULL, *errmsg = 0;
  mongo_collection *c;
  mongo_server *server;
  buffer buf;
  int bit_opts = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|z", &docs, &options) == FAILURE) {
    return;
  }

  /*
   * options are only supported in the new-style, ie: an array of "name parameters":
   * array("continueOnError" => true);
   */
  if (options) {
	  zval **continue_on_error = NULL;

	  zend_hash_find(HASH_P(options), "continueOnError", strlen("continueOnError")+1, (void**)&continue_on_error);
	  bit_opts = (continue_on_error ? Z_BVAL_PP(continue_on_error) : 0) << 0;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  if ((server = get_server(c TSRMLS_CC)) == 0) {
    RETURN_FALSE;
  }

  CREATE_BUF(buf, INITIAL_BUF_SIZE);

  if (php_mongo_write_batch_insert(&buf, Z_STRVAL_P(c->ns), bit_opts, docs,
                                   mongo_util_server_get_bson_size(server TSRMLS_CC) TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return;
  }

  SEND_MSG;

  efree(buf.start);
}

PHP_METHOD(MongoCollection, find) {
  zval *query = 0, *fields = 0;
  zend_bool slave_okay;
  mongo_collection *c;
  mongo_link *link;
  zval temp;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &query, &fields) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_COLLECTION(getThis());
  PHP_MONGO_GET_LINK(c->link);

  object_init_ex(return_value, mongo_ce_Cursor);

  // save & replace slave_okay
  slave_okay = link->slave_okay;
  link->slave_okay = c->slave_okay;

  if (!query) {
    MONGO_METHOD2(MongoCursor, __construct, &temp, return_value, c->link, c->ns);
  }
  else if (!fields) {
    MONGO_METHOD3(MongoCursor, __construct, &temp, return_value, c->link, c->ns, query);
  }
  else {
    MONGO_METHOD4(MongoCursor, __construct, &temp, return_value, c->link, c->ns, query, fields);
  }

  link->slave_okay = slave_okay;
}

PHP_METHOD(MongoCollection, findOne) {
  zval *query = 0, *fields = 0, *cursor;
  zval temp;
  zval *limit = &temp;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &query, &fields) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(cursor);
  MONGO_METHOD_BASE(MongoCollection, find)(ZEND_NUM_ARGS(), cursor, NULL, getThis(), 0 TSRMLS_CC);
  PHP_MONGO_CHECK_EXCEPTION1(&cursor);

  ZVAL_LONG(limit, -1);
  MONGO_METHOD1(MongoCursor, limit, cursor, cursor, limit);
  MONGO_METHOD(MongoCursor, getNext, return_value, cursor);

  zend_objects_store_del_ref(cursor TSRMLS_CC);
  zval_ptr_dtor(&cursor);
}

PHP_METHOD(MongoCollection, update) {
  zval *criteria, *newobj, *options = 0, *errmsg = 0;
  mongo_collection *c;
  mongo_server *server;
  buffer buf;
  int bit_opts = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|z", &criteria, &newobj, &options) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(criteria) || IS_SCALAR_P(newobj)) {
    zend_error(E_WARNING, "MongoCollection::update() expects parameters 1 and 2 to be arrays or objects");
    return;
  }
  /*
   * options could be a boolean (old-style, where "true" means "upsert") or an
   * array of "named parameters": array("upsert" => true, "multiple" => true)
   */

  if (options && !IS_SCALAR_P(options)) {
    zval **upsert = 0, **multiple = 0;

    zend_hash_find(HASH_P(options), "upsert", strlen("upsert")+1, (void**)&upsert);
    bit_opts = (upsert ? Z_BVAL_PP(upsert) : 0) << 0;

    zend_hash_find(HASH_P(options), "multiple", strlen("multiple")+1, (void**)&multiple);
    bit_opts |= (multiple ? Z_BVAL_PP(multiple) : 0) << 1;

    zval_add_ref(&options);
  }
  else {
    zval *opts;

    if (options && IS_SCALAR_P(options)) {
      zend_bool upsert = options ? Z_BVAL_P(options) : 0;
      bit_opts = upsert << 0;
    }

    MAKE_STD_ZVAL(opts);
    array_init(opts);
    options = opts;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  if ((server = get_server(c TSRMLS_CC)) == 0) {
    RETURN_FALSE;
  }

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (FAILURE == php_mongo_write_update(&buf, Z_STRVAL_P(c->ns), bit_opts, criteria, newobj TSRMLS_CC)) {
    efree(buf.start);
    zval_ptr_dtor(&options);
    return;
  }

  SEND_MSG;

  efree(buf.start);
  zval_ptr_dtor(&options);
}

PHP_METHOD(MongoCollection, remove) {
  zval *criteria = 0, *options = 0, *errmsg = 0;
  int flags = 0;
  mongo_collection *c;
  mongo_server *server;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &criteria, &options) == FAILURE) {
    return;
  }

  if (criteria && IS_SCALAR_P(criteria)) {
    zend_error(E_WARNING, "MongoCollection::remove() expects parameter 1 to be an array or object");
    return;
  }

  if (!criteria) {
    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
  }
  else {
    zval_add_ref(&criteria);
  }

  if (options && !IS_SCALAR_P(options)) {
    zval **just_one;

    if (zend_hash_find(HASH_P(options), "justOne", strlen("justOne")+1, (void**)&just_one) == SUCCESS) {
      flags = Z_BVAL_PP(just_one);
    }

    zval_add_ref(&options);
  }
  else {
    zval *opts;

    if (options && IS_SCALAR_P(options)) {
      flags = Z_BVAL_P(options);
    }

    MAKE_STD_ZVAL(opts);
    array_init(opts);
    options = opts;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  if ((server = get_server(c TSRMLS_CC)) == 0) {
    RETURN_FALSE;
  }

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (FAILURE == php_mongo_write_delete(&buf, Z_STRVAL_P(c->ns), flags, criteria TSRMLS_CC)) {
    efree(buf.start);
    zval_ptr_dtor(&options);
    zval_ptr_dtor(&criteria);
    return;
  }

  SEND_MSG;

  efree(buf.start);
  zval_ptr_dtor(&options);
  zval_ptr_dtor(&criteria);
}

PHP_METHOD(MongoCollection, ensureIndex) {
  zval *keys, *options = 0, *db, *system_indexes, *collection, *data, *key_str;
  mongo_collection *c;
  zend_bool done_name = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &keys, &options) == FAILURE) {
    return;
  }

  if (IS_SCALAR_P(keys)) {
    zval *key_array;

    convert_to_string(keys);

    if (Z_STRLEN_P(keys) == 0)
      return;

    MAKE_STD_ZVAL(key_array);
    array_init(key_array);
    add_assoc_long(key_array, Z_STRVAL_P(keys), 1);

    keys = key_array;
  }
  else {
    zval_add_ref(&keys);
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  // get the system.indexes collection
  db = c->parent;

  MAKE_STD_ZVAL(system_indexes);
  ZVAL_STRING(system_indexes, "system.indexes", 1);

  MAKE_STD_ZVAL(collection);
  MONGO_METHOD1(MongoDB, selectCollection, collection, db, system_indexes);
  PHP_MONGO_CHECK_EXCEPTION3(&keys, &system_indexes, &collection);

  // set up data
  MAKE_STD_ZVAL(data);
  array_init(data);

  // ns
  add_assoc_zval(data, "ns", c->ns);
  zval_add_ref(&c->ns);
  add_assoc_zval(data, "key", keys);
  zval_add_ref(&keys);

  // in ye olden days, "options" only had one option: unique
  // so, if we're parsing old-school code, "unique" is a boolean
  // in ye new days, "options" is an array.
  if (options) {

    // old-style
    if (IS_SCALAR_P(options)) {
      zval *opts;
      // assumes the person correctly passed in a boolean.  if they passed in a
      // string or something, it won't work and maybe they'll read the docs
      add_assoc_bool(data, "unique", Z_BVAL_P(options));

      MAKE_STD_ZVAL(opts);
      array_init(opts);
      options = opts;
    }
    // new style
    else {
      zval temp, **safe_pp, **fsync_pp, **timeout_pp, **name;
      zend_hash_merge(HASH_P(data), HASH_P(options), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);

      if (zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_pp) == SUCCESS) {
        zend_hash_del(HASH_P(data), "safe", strlen("safe")+1);
      }
      if (zend_hash_find(HASH_P(options), "fsync", strlen("fsync")+1, (void**)&fsync_pp) == SUCCESS) {
        zend_hash_del(HASH_P(data), "fsync", strlen("fsync")+1);
      }
      if (zend_hash_find(HASH_P(options), "timeout", strlen("timeout")+1, (void**)&timeout_pp) == SUCCESS) {
        zend_hash_del(HASH_P(data), "timeout", strlen("timeout")+1);
      }

      if (zend_hash_find(HASH_P(options), "name", strlen("name")+1, (void**)&name) == SUCCESS) {
        if (Z_TYPE_PP(name) == IS_STRING && Z_STRLEN_PP(name) > MAX_INDEX_NAME_LEN) {
          zval_ptr_dtor(&data);
          zend_throw_exception_ex(mongo_ce_Exception, 14 TSRMLS_CC, "index name too long: %d, max %d characters", Z_STRLEN_PP(name), MAX_INDEX_NAME_LEN);
          return;
        }
        done_name = 1;
      }
      zval_add_ref(&options);
    }
  }
  else {
    zval *opts;
    MAKE_STD_ZVAL(opts);
    array_init(opts);
    options = opts;
  }

  if (!done_name) {
    // turn keys into a string
    MAKE_STD_ZVAL(key_str);
    ZVAL_NULL(key_str);

    // MongoCollection::toIndexString()
    MONGO_METHOD1(MongoCollection, toIndexString, key_str, NULL, keys);

    if (Z_STRLEN_P(key_str) > MAX_INDEX_NAME_LEN) {
      zval_ptr_dtor(&data);
      zend_throw_exception_ex(mongo_ce_Exception, 14 TSRMLS_CC, "index name too long: %d, max %d characters", Z_STRLEN_P(key_str), MAX_INDEX_NAME_LEN);
      zval_ptr_dtor(&key_str);
      return;
    }

    add_assoc_zval(data, "name", key_str);
    zval_add_ref(&key_str);
  }

  // MongoCollection::insert()
  MONGO_METHOD2(MongoCollection, insert, return_value, collection, data, options);

  zval_ptr_dtor(&options);
  zval_ptr_dtor(&data);
  zval_ptr_dtor(&system_indexes);
  zval_ptr_dtor(&collection);
  zval_ptr_dtor(&keys);

  if (!done_name) {
    zval_ptr_dtor(&key_str);
  }
}

PHP_METHOD(MongoCollection, deleteIndex) {
  zval *keys, *key_str, *data;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &keys) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(key_str);
  MONGO_METHOD1(MongoCollection, toIndexString, key_str, NULL, keys);

  PHP_MONGO_GET_COLLECTION(getThis());

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_zval(data, "deleteIndexes", c->name);
  zval_add_ref(&c->name);
  add_assoc_zval(data, "index", key_str);

  MONGO_CMD(return_value, c->parent);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, deleteIndexes) {
  zval *data;
  mongo_collection *c;
  PHP_MONGO_GET_COLLECTION(getThis());

  MAKE_STD_ZVAL(data);
  array_init(data);

  add_assoc_string(data, "deleteIndexes", Z_STRVAL_P(c->name), 1);
  add_assoc_string(data, "index", "*", 1);

  MONGO_CMD(return_value, c->parent);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, getIndexInfo) {
  zval *collection, *i_str, *query, *cursor, *next;
  mongo_collection *c;
  PHP_MONGO_GET_COLLECTION(getThis());

  MAKE_STD_ZVAL(collection);

  MAKE_STD_ZVAL(i_str);
  ZVAL_STRING(i_str, "system.indexes", 1);
  MONGO_METHOD1(MongoDB, selectCollection, collection, c->parent, i_str);
  zval_ptr_dtor(&i_str);
  PHP_MONGO_CHECK_EXCEPTION1(&collection);

  MAKE_STD_ZVAL(query);
  array_init(query);
  add_assoc_string(query, "ns", Z_STRVAL_P(c->ns), 1);

  MAKE_STD_ZVAL(cursor);
  MONGO_METHOD1(MongoCollection, find, cursor, collection, query);
  PHP_MONGO_CHECK_EXCEPTION3(&collection, &query, &cursor);

  zval_ptr_dtor(&query);
  zval_ptr_dtor(&collection);

  array_init(return_value);

  MAKE_STD_ZVAL(next);
  MONGO_METHOD(MongoCursor, getNext, next, cursor);
  PHP_MONGO_CHECK_EXCEPTION2(&cursor, &next);
  while (Z_TYPE_P(next) != IS_NULL) {
    add_next_index_zval(return_value, next);

    MAKE_STD_ZVAL(next);
    MONGO_METHOD(MongoCursor, getNext, next, cursor);
    PHP_MONGO_CHECK_EXCEPTION2(&cursor, &next);
  }
  zval_ptr_dtor(&next);
  zval_ptr_dtor(&cursor);
}

PHP_METHOD(MongoCollection, count) {
  zval *response, *data, *query=0;
  long limit = 0, skip = 0;
  zval **n;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zll", &query, &limit, &skip) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "count", Z_STRVAL_P(c->name), 1);
  if (query) {
    add_assoc_zval(data, "query", query);
    zval_add_ref(&query);
  }
  if (limit) {
    add_assoc_long(data, "limit", limit);
  }
  if (skip) {
    add_assoc_long(data, "skip", skip);
  }

  MAKE_STD_ZVAL(response);
  ZVAL_NULL(response);

  MONGO_CMD(response, c->parent);

  zval_ptr_dtor(&data);

  if (EG(exception) || Z_TYPE_P(response) != IS_ARRAY) {
    zval_ptr_dtor(&response);
    return;
  }

  if (zend_hash_find(HASH_P(response), "n", 2, (void**)&n) == SUCCESS) {
    convert_to_long(*n);
    RETVAL_ZVAL(*n, 1, 0);
    zval_ptr_dtor(&response);
  }
  else {
    RETURN_ZVAL(response, 0, 0);
  }
}

PHP_METHOD(MongoCollection, save) {
  zval *a, *options = 0;
  zval **id;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &a, &options) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(a) || (options && IS_SCALAR_P(options))) {
    zend_error(E_WARNING, "MongoCollection::save() expects parameters 1 and 2 to be arrays or objects");
    return;
  }

  if (!options) {
    MAKE_STD_ZVAL(options);
    array_init(options);
  }
  else {
    zval_add_ref(&options);
  }

  if (zend_hash_find(HASH_P(a), "_id", 4, (void**)&id) == SUCCESS) {
    zval *criteria;

    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
    add_assoc_zval(criteria, "_id", *id);
    zval_add_ref(id);

    add_assoc_bool(options, "upsert", 1);

    MONGO_METHOD3(MongoCollection, update, return_value, getThis(), criteria, a, options);

    zval_ptr_dtor(&criteria);
    zval_ptr_dtor(&options);
    return;
  }

  MONGO_METHOD2(MongoCollection, insert, return_value, getThis(), a, options);
  zval_ptr_dtor(&options);
}

PHP_METHOD(MongoCollection, createDBRef) {
  zval *obj;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &obj) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_COLLECTION(getThis());
  MONGO_METHOD2(MongoDB, createDBRef, return_value, c->parent, c->name, obj);
}

PHP_METHOD(MongoCollection, getDBRef) {
  zval *ref;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_COLLECTION(getThis());
  MONGO_METHOD2(MongoDBRef, get, return_value, NULL, c->parent, ref);
}

static char *replace_dots(char *key, int key_len, char *position) {
  int i;
  for (i=0; i<key_len; i++) {
    if (key[i] == '.') {
      *(position)++ = '_';
    }
    else {
      *(position)++ = key[i];
    }
  }
  return position;
}

/* {{{ MongoCollection::toIndexString(array|string) */
PHP_METHOD(MongoCollection, toIndexString) {
  zval *zkeys;
  char *name, *position;
  int len = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zkeys) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(zkeys) == IS_ARRAY ||
      Z_TYPE_P(zkeys) == IS_OBJECT) {
    HashTable *hindex = HASH_P(zkeys);
    HashPosition pointer;
    zval **data;
    char *key;
    uint key_len, first = 1, key_type;
    ulong index;

    for(zend_hash_internal_pointer_reset_ex(hindex, &pointer);
        zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS;
        zend_hash_move_forward_ex(hindex, &pointer)) {

      key_type = zend_hash_get_current_key_ex(hindex, &key, &key_len, &index, NO_DUP, &pointer);
      switch (key_type) {
      case HASH_KEY_IS_STRING: {
        len += key_len;

        if (Z_TYPE_PP(data) == IS_STRING) {
          len += Z_STRLEN_PP(data)+1;
        }
        else {
          len += Z_LVAL_PP(data) == 1 ? 2 : 3;
        }

        break;
      }
      case HASH_KEY_IS_LONG:
        convert_to_string(*data);

        len += Z_STRLEN_PP(data);
        len += 2;
        break;
      default:
        continue;
      }
    }

    name = (char*)emalloc(len+1);
    position = name;

    for(zend_hash_internal_pointer_reset_ex(hindex, &pointer);
        zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS;
        zend_hash_move_forward_ex(hindex, &pointer)) {

      if (!first) {
        *(position)++ = '_';
      }
      first = 0;

      key_type = zend_hash_get_current_key_ex(hindex, &key, &key_len, &index, NO_DUP, &pointer);

      if (key_type == HASH_KEY_IS_LONG) {
        key_len = spprintf(&key, 0, "%ld", index);
        key_len += 1;
      }

      // copy str, replacing '.' with '_'
      position = replace_dots(key, key_len-1, position);

      *(position)++ = '_';

      if (Z_TYPE_PP(data) == IS_STRING) {
        memcpy(position, Z_STRVAL_PP(data), Z_STRLEN_PP(data));
        position += Z_STRLEN_PP(data);
      }
      else {
        if (Z_LVAL_PP(data) != 1) {
          *(position)++ = '-';
        }
        *(position)++ = '1';
      }

      if (key_type == HASH_KEY_IS_LONG) {
        efree(key);
      }
    }
    *(position) = 0;
  }
  else {
    int len;
    convert_to_string(zkeys);

    len = Z_STRLEN_P(zkeys);

    name = (char*)emalloc(len + 3);
    position = name;

    // copy str, replacing '.' with '_'
    position = replace_dots(Z_STRVAL_P(zkeys), Z_STRLEN_P(zkeys), position);

    *(position)++ = '_';
    *(position)++ = '1';
    *(position) = '\0';
  }
  RETURN_STRING(name, 0)
}


/* {{{ MongoCollection::group
 */
PHP_METHOD(MongoCollection, group) {
  zval *key, *initial, *options = 0, *group, *data, *reduce;
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzz|z", &key, &initial, &reduce, &options) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(reduce) == IS_STRING) {
    zval *code;
    MAKE_STD_ZVAL(code);
    object_init_ex(code, mongo_ce_Code);

    MONGO_METHOD1(MongoCode, __construct, return_value, code, reduce);

    reduce = code;
  }
  else {
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
  }
  else if (!IS_SCALAR_P(key)) {
    add_assoc_zval(group, "key", key);
  }
  else {
    zval_ptr_dtor(&group);
    zval_ptr_dtor(&reduce);
    zend_throw_exception(mongo_ce_Exception, "MongoCollection::group takes an array, object, or MongoCode key", 0 TSRMLS_CC);
    return;
  }
  zval_add_ref(&key);

  /*
   * options used to just be "condition" but now can be "condition" or "finalize"
   */
  if (options) {
    zval **condition = 0, **finalize = 0;

    // new case
    if (zend_hash_find(HASH_P(options), "condition", strlen("condition")+1, (void**)&condition) == SUCCESS) {
      add_assoc_zval(group, "cond", *condition);
      zval_add_ref(condition);
    }
    if (zend_hash_find(HASH_P(options), "finalize", strlen("finalize")+1, (void**)&finalize) == SUCCESS) {
      add_assoc_zval(group, "finalize", *finalize);
      zval_add_ref(finalize);
    }
    if (!condition && !finalize) {
      add_assoc_zval(group, "cond", options);
      zval_add_ref(&options);
    }
  }

  add_assoc_zval(group, "initial", initial);
  zval_add_ref(&initial);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_zval(data, "group", group);

  MONGO_CMD(return_value, c->parent);

  zval_ptr_dtor(&data);
  zval_ptr_dtor(&reduce);
}
/* }}} */

/* {{{ MongoCollection::__get
 */
PHP_METHOD(MongoCollection, __get) {
  /*
   * this is a little trickier than the getters in Mongo and MongoDB... we need
   * to combine the current collection name with the parameter passed in, get
   * the parent db, then select the new collection from it.
   */
  zval *name, *full_name;
  char *full_name_s;
  mongo_collection *c;
  PHP_MONGO_GET_COLLECTION(getThis());

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
    return;
  }

  /*
   * If this is "db", return the parent database.  This can't actually be a
   * property of the obj because apache does weird things on object destruction
   * that will cause the link to be destroyed twice.
   */
  if (strcmp(Z_STRVAL_P(name), "db") == 0) {
    RETURN_ZVAL(c->parent, 1, 0);
  }

  spprintf(&full_name_s, 0, "%s.%s", Z_STRVAL_P(c->name), Z_STRVAL_P(name));
  MAKE_STD_ZVAL(full_name);
  ZVAL_STRING(full_name, full_name_s, 0);

  // select this collection
  MONGO_METHOD1(MongoDB, selectCollection, return_value, c->parent, full_name);

  zval_ptr_dtor(&full_name);
}
/* }}} */


static zend_function_entry MongoCollection_methods[] = {
  PHP_ME(MongoCollection, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, __toString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, __get, arginfo___get, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, getSlaveOkay, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, setSlaveOkay, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, drop, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, validate, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, insert, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, batchInsert, arginfo_batchInsert, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, update, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, remove, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, find, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, findOne, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, ensureIndex, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, deleteIndex, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, deleteIndexes, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, getIndexInfo, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, count, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, save, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, createDBRef, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, getDBRef, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, toIndexString, NULL, ZEND_ACC_PROTECTED|ZEND_ACC_STATIC)
  PHP_ME(MongoCollection, group, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

static void php_mongo_collection_free(void *object TSRMLS_DC) {
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
    zend_object_std_dtor(&c->std TSRMLS_CC);
    efree(c);
  }
}


/* {{{ php_mongo_collection_new
 */
zend_object_value php_mongo_collection_new(zend_class_entry *class_type TSRMLS_DC) {
  php_mongo_obj_new(mongo_collection);
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
  zend_declare_property_long(mongo_ce_Collection, "wtimeout", strlen("wtimeout"), PHP_MONGO_DEFAULT_TIMEOUT, ZEND_ACC_PUBLIC TSRMLS_CC);
}
