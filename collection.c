//collection.c
/**
 *  Copyright 2009-2010 10gen, Inc.
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

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_DB,
  *mongo_ce_Cursor,
  *mongo_ce_Code,
  *mongo_ce_Exception,
  *mongo_ce_CursorException;

extern int le_pconnection,
  le_connection;

extern zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_Collection = NULL;

static int safe_op(mongo_link *link, zval *cursor_z, buffer *buf, zval *return_value TSRMLS_DC);
static zval* append_getlasterror(zval *coll, buffer *buf, int safe, int fsync TSRMLS_DC);
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


PHP_METHOD(MongoCollection, __construct) {
  zval *parent, *name, *zns, *w, *wtimeout;
  mongo_collection *c;
  mongo_db *db;
  char *ns;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, mongo_ce_DB, &name) == FAILURE) {
    return;
  }
  convert_to_string(name);

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);

  PHP_MONGO_GET_DB(parent);

  c->link = db->link;
  zval_add_ref(&db->link);

  c->parent = parent;
  zval_add_ref(&parent);

  c->name = name;
  zval_add_ref(&name);

  spprintf(&ns, 0, "%s.%s", Z_STRVAL_P(db->name), Z_STRVAL_P(name));

  MAKE_STD_ZVAL(zns);
  ZVAL_STRING(zns, ns, 0);
  c->ns = zns;

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
static zval* append_getlasterror(zval *coll, buffer *buf, int safe, int fsync TSRMLS_DC) {
  zval *cmd_ns_z, *cmd, *cursor_z, *temp;
  char *cmd_ns;
  mongo_cursor *cursor;
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(coll TSRMLS_CC);
  mongo_db *db = (mongo_db*)zend_object_store_get_object(c->parent TSRMLS_CC);
  int response;

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

  if (safe > 1) {
    zval *wtimeout;

    add_assoc_long(cmd, "w", safe); 

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

static int safe_op(mongo_link *link, zval *cursor_z, buffer *buf, zval *return_value TSRMLS_DC) {
  zval *errmsg, **err;
  mongo_cursor *cursor;
  int response;

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  // send everything
  response = mongo_say(link, buf, errmsg TSRMLS_CC);

  if (FAILURE == response) {
    zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_P(errmsg), 0 TSRMLS_CC);
    zval_ptr_dtor(&errmsg);
    zval_ptr_dtor(&cursor_z);
    return FAILURE;
  }

  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_z TSRMLS_CC);

  // get reply
  response = php_mongo_get_reply(cursor, errmsg TSRMLS_CC);
  if (FAILURE == response) {
    zval_ptr_dtor(&errmsg);
    zval_ptr_dtor(&cursor_z);
    return FAILURE;
  }
  zval_ptr_dtor(&errmsg);

  cursor->started_iterating = 1;

  MONGO_METHOD(MongoCursor, getNext, return_value, cursor_z);

  zval_ptr_dtor(&cursor_z);

  if (EG(exception) || 
      (Z_TYPE_P(return_value) ==IS_BOOL && Z_BVAL_P(return_value) == 0)) {
    return FAILURE;
  }
  /* if getlasterror returned an error, throw an exception 
   *
   * this isn't the same as checking for $err in cursor.c, as this isn't a query
   * error but just the status.
   */
  else if (zend_hash_find(Z_ARRVAL_P(return_value), "err", strlen("err")+1, (void**)&err) == SUCCESS &&
      Z_TYPE_PP(err) == IS_STRING) {
    zval **code_z;
    int code = 10;

    // get error code
    if (zend_hash_find(Z_ARRVAL_P(return_value), "code", strlen("code")+1, (void**)&code_z) == SUCCESS) {
      code = Z_LVAL_PP(code_z);
    }

    zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_PP(err), code TSRMLS_CC);
    return FAILURE;
  }
  // w timeout
  else if (zend_hash_find(Z_ARRVAL_P(return_value), "errmsg", strlen("errmsg")+1, (void**)&err) == SUCCESS &&
      Z_TYPE_PP(err) == IS_STRING) {
    zval **code;
    int status = zend_hash_find(Z_ARRVAL_P(return_value), "n", strlen("n")+1, (void**)&code);

    zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_PP(err), 
                         (status == SUCCESS ? Z_LVAL_PP(code) : 0) TSRMLS_CC);
    return FAILURE;
  }

  return SUCCESS;
}


PHP_METHOD(MongoCollection, insert) {
  zval *a, *options = 0;
  int safe = 0, fsync = 0;
  mongo_collection *c;
  mongo_link *link;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &a, &options) == FAILURE) {
    return;
  }

  if (IS_SCALAR_P(a)) {
    zend_error(E_WARNING, "MongoCollection::insert() expects parameter 1 to be an array or object");
    return;
  }

  if (options) {
    if (!IS_SCALAR_P(options)) {
      zval **safe_pp, **fsync_pp;
      if (SUCCESS == zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_pp)) {
        safe = Z_LVAL_PP(safe_pp);
      }
      if (SUCCESS == zend_hash_find(HASH_P(options), "fsync", strlen("fysnc")+1, (void**)&fsync_pp)) {
        fsync = Z_BVAL_PP(fsync_pp);
        if (fsync && !safe) {
          safe = 1;
        }
      }
    }
    // old boolean options
    else {
      safe = Z_BVAL_P(options);
    }
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (FAILURE == php_mongo_write_insert(&buf, Z_STRVAL_P(c->ns), a TSRMLS_CC)) {
    efree(buf.start);
    RETURN_FALSE;
  }

  SEND_MSG;

  efree(buf.start);
}

PHP_METHOD(MongoCollection, batchInsert) {
  zval *docs, *options = 0;
  int safe = 0, fsync = 0;
  mongo_collection *c;
  mongo_link *link;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|z", &docs, &options) == FAILURE) {
    return;
  }

  GET_SAFE_OPTION;

  PHP_MONGO_GET_COLLECTION(getThis());

  CREATE_BUF(buf, INITIAL_BUF_SIZE);

  if (php_mongo_write_batch_insert(&buf, Z_STRVAL_P(c->ns), docs TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return;
  }

  SEND_MSG;

  efree(buf.start);
}

PHP_METHOD(MongoCollection, find) {
  zval *query = 0, *fields = 0;
  mongo_collection *c;
  zval temp;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &query, &fields) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  object_init_ex(return_value, mongo_ce_Cursor);

  if (!query) {
    MONGO_METHOD2(MongoCursor, __construct, &temp, return_value, c->link, c->ns);
  }
  else if (!fields) {
    MONGO_METHOD3(MongoCursor, __construct, &temp, return_value, c->link, c->ns, query);
  }
  else {
    MONGO_METHOD4(MongoCursor, __construct, &temp, return_value, c->link, c->ns, query, fields);
  }
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
  zval *criteria, *newobj, *options = 0;
  mongo_collection *c;
  mongo_link *link;
  buffer buf;
  int safe = 0, fsync = 0, opts = 0;

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

  // old-style
  if (!options || IS_SCALAR_P(options)) {
    zend_bool upsert = options ? Z_BVAL_P(options) : 0;
    opts = upsert << 0;
  }
  // new-style
  else {
    zval **upsert = 0, **multiple = 0, **safe_pp = 0, **fsync_pp;

    zend_hash_find(HASH_P(options), "upsert", strlen("upsert")+1, (void**)&upsert);
    opts = (upsert ? Z_BVAL_PP(upsert) : 0) << 0;

    zend_hash_find(HASH_P(options), "multiple", strlen("multiple")+1, (void**)&multiple);
    opts |= (multiple ? Z_BVAL_PP(multiple) : 0) << 1;

    if (zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_pp) == SUCCESS) {
      safe = Z_BVAL_PP(safe_pp);
    }
    if (zend_hash_find(HASH_P(options), "fsync", strlen("fsync")+1, (void**)&fsync_pp) == SUCCESS) {
      fsync = Z_BVAL_PP(fsync_pp);
      if (fsync && !safe) {
        safe = 1;
      }
    }
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (FAILURE == php_mongo_write_update(&buf, Z_STRVAL_P(c->ns), opts, criteria, newobj TSRMLS_CC)) {
    efree(buf.start);
    return;
  }

  SEND_MSG;

  efree(buf.start);
}

PHP_METHOD(MongoCollection, remove) {
  zval *criteria = 0, *options = 0;
  int flags = 0, safe = 0, fsync = 0;
  mongo_collection *c;
  mongo_link *link;
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

  if (options) {
    if (IS_SCALAR_P(options)) {
      flags = Z_BVAL_P(options);
    }
    else {
      zval **just_one, **safe_pp, **fsync_pp;

      if (zend_hash_find(HASH_P(options), "justOne", strlen("justOne")+1, (void**)&just_one) == SUCCESS) {
        flags = Z_BVAL_PP(just_one);
      }
      if (zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_pp) == SUCCESS) {
        safe = Z_BVAL_PP(safe_pp);
      }
      if (zend_hash_find(HASH_P(options), "fsync", strlen("fsync")+1, (void**)&fsync_pp) == SUCCESS) {
        fsync = Z_BVAL_PP(fsync_pp);
        if (fsync && !safe) {
          safe = 1;
        }
      }
    }
  }

  PHP_MONGO_GET_COLLECTION(getThis());

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (FAILURE == php_mongo_write_delete(&buf, Z_STRVAL_P(c->ns), flags, criteria TSRMLS_CC)) {
    efree(buf.start);
    zval_ptr_dtor(&criteria);
    return;
  }

  SEND_MSG;

  efree(buf.start);
  zval_ptr_dtor(&criteria);
}

PHP_METHOD(MongoCollection, ensureIndex) {
  zval *keys, *options = 0, *db, *system_indexes, *collection, *data, *key_str, *safe_insert = 0;
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

  /*
   * in ye olden days, "options" only had one options: unique
   * so, if we're parsing old-school code, "unique" is a boolean
   * in ye new days, "options is an array with possible keys
   * "unique" and "dropDups".
   */
  if (options) {

    // old-style
    if (IS_SCALAR_P(options)) {
      /* 
       * assumes the person correctly passed in a boolean.  if they passed in a
       * string or something, it won't work and maybe they'll read the docs
       */
      add_assoc_zval(data, "unique", options);
      // and, since we'll be destroying data later:
      zval_add_ref(&options);
    }
    // new style
    else {
      zval temp, **safe_pp, **name;
      zend_hash_merge(HASH_P(data), HASH_P(options), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);

      // old safe insert syntax
      if (zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_pp) == SUCCESS) {
        if (Z_BVAL_PP(safe_pp)) {
          MAKE_STD_ZVAL(safe_insert);
          ZVAL_BOOL(safe_insert, 1);
        }
        zend_hash_del(HASH_P(data), "safe", strlen("safe")+1);
      }
      if (zend_hash_find(HASH_P(options), "name", strlen("name")+1, (void**)&name) == SUCCESS) {
        done_name = 1;
      }
    }
  }

  if (!done_name) {
    // turn keys into a string
    MAKE_STD_ZVAL(key_str);
        
    // MongoCollection::toIndexString()
    MONGO_METHOD1(MongoCollection, toIndexString, key_str, NULL, keys);
        
    add_assoc_zval(data, "name", key_str);
    zval_add_ref(&key_str);
  }

  // MongoCollection::insert()
  if (safe_insert) {
    MONGO_METHOD2(MongoCollection, insert, return_value, collection, data, safe_insert);
    zval_ptr_dtor(&safe_insert);
  }
  else {
    MONGO_METHOD1(MongoCollection, insert, return_value, collection, data);
  }

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
  zval *response, *data, *query=0, *limit = 0, *skip = 0;
  zval **n;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zzz", &query, &limit, &skip) == FAILURE) {
    return;
  }

  if ((limit && Z_TYPE_P(limit) != IS_LONG) || 
      (skip && Z_TYPE_P(skip) != IS_LONG)) {
#if ZEND_MODULE_API_NO >= 20060613
    zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "MongoCollection::count(): limit and skip must be ints");
#else
    zend_throw_exception_ex(zend_exception_get_default(), 0 TSRMLS_CC, "MongoCollection::count(): limit and skip must be ints");
#endif /* ZEND_MODULE_API_NO >= 20060613 */
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
    add_assoc_zval(data, "limit", limit);
    zval_add_ref(&limit);
  }
  if (skip) {
    add_assoc_zval(data, "skip", skip);
    zval_add_ref(&skip);
  }
  
  MAKE_STD_ZVAL(response);
  ZVAL_NULL(response);
  
  MONGO_CMD(response, c->parent);

  zval_ptr_dtor(&data);

  if (EG(exception)) {
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


static function_entry MongoCollection_methods[] = {
  PHP_ME(MongoCollection, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, __toString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, __get, arginfo___get, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, drop, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, validate, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, insert, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, batchInsert, NULL, ZEND_ACC_PUBLIC)
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
