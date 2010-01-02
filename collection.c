//collection.c
/**
 *  Copyright 2009 10gen, Inc.
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

/* 
 * arginfo needs to be set for __get because if PHP doesn't know it only takes
 * one arg, it will issue a warning.
 */
#if ZEND_MODULE_API_NO <= 20060613
static
#endif
ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()


PHP_METHOD(MongoCollection, __construct) {
  zval *db, *name, *zns;
  mongo_collection *c;
  char *ns;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &db, mongo_ce_DB, &name) == FAILURE) {
    return;
  }
  convert_to_string(name);

  zend_update_property(mongo_ce_Collection, getThis(), "db", strlen("db"), db TSRMLS_CC);

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);

  c->parent = db;
  zval_add_ref(&c->parent);

  c->db = (mongo_db*)zend_object_store_get_object(db TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->db->name, MongoDB);

  c->name = name;
  zval_add_ref(&name);

  spprintf(&ns, 0, "%s.%s", Z_STRVAL_P(c->db->name), Z_STRVAL_P(name));

  MAKE_STD_ZVAL(zns);
  ZVAL_STRING(zns, ns, 0);
  c->ns = zns;
}

PHP_METHOD(MongoCollection, __toString) {
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED_STRING(c->ns, MongoCollection);
  RETURN_ZVAL(c->ns, 1, 0);
}

PHP_METHOD(MongoCollection, getName) {
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);
  RETURN_ZVAL(c->name, 1, 0);
}

PHP_METHOD(MongoCollection, drop) {
  zval *data;
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

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

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "validate", Z_STRVAL_P(c->name), 1);
  add_assoc_bool(data, "scandata", scan_data);

  MONGO_CMD(return_value, c->parent);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, insert) {
  zval *temp, *a, *options = 0;
  zend_bool safe = 0;
  mongo_collection *c;
  mongo_link *link;
  int response;
  mongo_msg_header header;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &a, &options) == FAILURE ||
      IS_SCALAR_P(a)) {
    return;
  }


  if (options) {
    zval **safe_z;
    // new, just in case there are more options coming
    if (!IS_SCALAR_P(options) && zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_z) == SUCCESS && Z_BVAL_PP(safe_z)) {
      safe = 1;
    }
    // old boolean options
    else {
      safe = Z_BVAL_P(options);
    }
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  link = (mongo_link*)zend_object_store_get_object(c->db->link TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(link->server_set, Mongo);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(c->ns), OP_INSERT);

  // serialize
  if (zval_to_bson(&buf, HASH_P(a), PREP TSRMLS_CC) == 0 &&
      zend_hash_num_elements(HASH_P(a)) == 0) {
    efree(buf.start);
    // return if there were 0 elements
    RETURN_FALSE;
  }

  // throw an exception if the obj was too big
  if(buf.pos - buf.start > MAX_OBJECT_LEN) {
    char *msg;

    spprintf(&msg, 0, "size of BSON is %d bytes, max 4MB", buf.pos-buf.start);
    zend_throw_exception(mongo_ce_Exception, msg, 0 TSRMLS_CC);
    efree(msg);

    efree(buf.start);
    return;
  }

  MAKE_STD_ZVAL(temp);
  ZVAL_NULL(temp);

  php_mongo_serialize_size(buf.start, &buf);

  if (safe) {
    int start = buf.pos - buf.start;
    zval *cmd, *cursor_z, *cmd_ns_z, **err;
    char *cmd_ns;
    mongo_cursor *cursor;

    MAKE_STD_ZVAL(cmd_ns_z);

    spprintf(&cmd_ns, 0, "%s.$cmd", Z_STRVAL_P(c->db->name));
    /* add a query */
    CREATE_HEADER(buf, cmd_ns, OP_QUERY);
    ZVAL_STRING(cmd_ns_z, cmd_ns, 0);

    php_mongo_serialize_int(&buf, 0);
    php_mongo_serialize_int(&buf, -1);

    MAKE_STD_ZVAL(cmd);
    array_init(cmd);
    add_assoc_long(cmd, "getlasterror", 1);

    zval_to_bson(&buf, HASH_P(cmd), NO_PREP TSRMLS_CC);

    php_mongo_serialize_size(buf.start + start, &buf);

    zval_ptr_dtor(&cmd);

    /* send everything */
    response = mongo_say(link, &buf, temp TSRMLS_CC);
    efree(buf.start);
    if (response == FAILURE) {
      zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_P(temp), 0 TSRMLS_CC);
      zval_ptr_dtor(&cmd_ns_z);
      zval_ptr_dtor(&temp);
      return;
    }

    MAKE_STD_ZVAL(cursor_z);
    object_init_ex(cursor_z, mongo_ce_Cursor);
    MONGO_METHOD2(MongoCursor, __construct, temp, cursor_z, c->db->link, cmd_ns_z);

    /* get the response */
    cursor = (mongo_cursor*)zend_object_store_get_object(cursor_z TSRMLS_CC);
    cursor->send.request_id = header.request_id;
    if (php_mongo_get_reply(cursor, temp TSRMLS_CC) == FAILURE) {
      zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_P(temp), 0 TSRMLS_CC);
      zval_ptr_dtor(&temp);
      return;
    }

    MONGO_METHOD(MongoCursor, getNext, return_value, cursor_z);

    zval_ptr_dtor(&cursor_z);
    zval_ptr_dtor(&cmd_ns_z);

    /* if getlasterror returned an error, throw an exception */
    zend_hash_find(Z_ARRVAL_P(return_value), "err", strlen("err")+1, (void**)&err);
    if (Z_TYPE_PP(err) == IS_STRING) {
      zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_PP(err), 0 TSRMLS_CC);
      return;
    }
  }
  else {
    response = mongo_say(link, &buf, temp TSRMLS_CC);
    efree(buf.start);
  
    RETVAL_BOOL(response >= SUCCESS);
  }
  zval_ptr_dtor(&temp);
}

PHP_METHOD(MongoCollection, batchInsert) {
  zval temp, *a, **data;
  mongo_collection *c;
  mongo_link *link;
  HashTable *php_array;
  int count = 0, start = 0;
  HashPosition pointer;
  mongo_msg_header header;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &a) == FAILURE) {
    return;
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  link = (mongo_link*)zend_object_store_get_object(c->db->link TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(link->server_set, Mongo);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(c->ns), OP_INSERT);

  php_array = HASH_P(a);

  for(zend_hash_internal_pointer_reset_ex(php_array, &pointer); 
      zend_hash_get_current_data_ex(php_array, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(php_array, &pointer)) {

    if(IS_SCALAR_PP(data)) {
      continue;
    }

    start = buf.pos-buf.start;
    zval_to_bson(&buf, HASH_PP(data), PREP TSRMLS_CC);

    // throw an exception if the obj was too big
    if(buf.pos - (buf.start+start) > MAX_OBJECT_LEN) {
      char *msg;
      
      spprintf(&msg, 0, "size of %dth BSON is %d bytes, max 4MB", count, buf.pos-buf.start);
      zend_throw_exception(mongo_ce_Exception, msg, 0 TSRMLS_CC);
      efree(msg);
      
      efree(buf.start);
      return;
    }

    php_mongo_serialize_size(buf.start+start, &buf);

    count++;
  }

  // if there are no elements, don't bother saving
  if (count == 0) {
    efree(buf.start);
    RETURN_FALSE;
  }

  php_mongo_serialize_size(buf.start, &buf);

  RETVAL_BOOL(mongo_say(link, &buf, &temp TSRMLS_CC)+1);
  efree(buf.start);
}

PHP_METHOD(MongoCollection, find) {
  zval *query = 0, *fields = 0;
  mongo_collection *c;
  zval temp;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &query, &fields) == FAILURE ||
      (ZEND_NUM_ARGS() > 0 && IS_SCALAR_P(query)) ||
      (ZEND_NUM_ARGS() > 1 && IS_SCALAR_P(fields))) {
    return;
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  object_init_ex(return_value, mongo_ce_Cursor);

  if (!query) {
    MONGO_METHOD2(MongoCursor, __construct, &temp, return_value, c->db->link, c->ns);
  }
  else if (!fields) {
    MONGO_METHOD3(MongoCursor, __construct, &temp, return_value, c->db->link, c->ns, query);
  }
  else {
    MONGO_METHOD4(MongoCursor, __construct, &temp, return_value, c->db->link, c->ns, query, fields);
  }
}

PHP_METHOD(MongoCollection, findOne) {
  zval *query = 0, *fields = 0, *cursor;
  zval temp;
  zval *limit = &temp;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &query, &fields) == FAILURE ||
      (ZEND_NUM_ARGS() > 0 && IS_SCALAR_P(query)) ||
      (ZEND_NUM_ARGS() > 1 && IS_SCALAR_P(fields))) {
    return;
  }

  MAKE_STD_ZVAL(cursor);
  MONGO_METHOD_BASE(MongoCollection, find)(ZEND_NUM_ARGS(), cursor, NULL, getThis(), 0 TSRMLS_CC);

  ZVAL_LONG(limit, -1);
  MONGO_METHOD1(MongoCursor, limit, cursor, cursor, limit);
  MONGO_METHOD(MongoCursor, getNext, return_value, cursor);

  zend_objects_store_del_ref(cursor TSRMLS_CC);
  zval_ptr_dtor(&cursor);
}

PHP_METHOD(MongoCollection, update) {
  zval temp, *criteria, *newobj, *options = 0;
  mongo_collection *c;
  mongo_link *link;
  mongo_msg_header header;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|z", &criteria, &newobj, &options) == FAILURE ||
      IS_SCALAR_P(criteria) ||
      IS_SCALAR_P(newobj)) {
    return;
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  link = (mongo_link*)zend_object_store_get_object(c->db->link TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(link->server_set, Mongo);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(c->ns), OP_UPDATE);

  /* 
   * options could be a boolean (old-style, where "true" means "upsert") or an 
   * array of "named parameters": array("upsert" => true, "multiple" => true)
   */

  // old-style
  if (!options || IS_SCALAR_P(options)) {
    zend_bool upsert = options ? Z_BVAL_P(options) : 0;
    php_mongo_serialize_int(&buf, upsert << 0);
  }
  // new-style
  else {
    zval **upsert = 0, **multiple = 0;

    zend_hash_find(HASH_P(options), "upsert", strlen("upsert")+1, (void**)&upsert);
    zend_hash_find(HASH_P(options), "multiple", strlen("multiple")+1, (void**)&multiple);
    php_mongo_serialize_int(&buf, 
        ((upsert ? Z_BVAL_PP(upsert) : 0) << 0) | ((multiple ? Z_BVAL_PP(multiple) : 0) << 1));
  }

  zval_to_bson(&buf, HASH_P(criteria), NO_PREP TSRMLS_CC);
  zval_to_bson(&buf, HASH_P(newobj), NO_PREP TSRMLS_CC);
  php_mongo_serialize_size(buf.start, &buf);

  RETVAL_BOOL(mongo_say(link, &buf, &temp TSRMLS_CC)+1);
  efree(buf.start);
}

PHP_METHOD(MongoCollection, remove) {
  zval temp, *criteria = 0;
  zend_bool just_one = 0;
  mongo_collection *c;
  mongo_link *link;
  int mflags;
  mongo_msg_header header;
  buffer buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zb", &criteria, &just_one) == FAILURE ||
      (ZEND_NUM_ARGS() > 0 && IS_SCALAR_P(criteria))) {
    return;
  }

  if (!criteria) {
    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
  }
  else {
    zval_add_ref(&criteria);
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  link = (mongo_link*)zend_object_store_get_object(c->db->link TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(link->server_set, Mongo);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(c->ns), OP_DELETE);

  mflags = (just_one == 1);

  php_mongo_serialize_int(&buf, mflags);
  zval_to_bson(&buf, HASH_P(criteria), NO_PREP TSRMLS_CC);
  php_mongo_serialize_size(buf.start, &buf);

  RETVAL_BOOL(mongo_say(link, &buf, &temp TSRMLS_CC)+1);

  efree(buf.start);
  zval_ptr_dtor(&criteria);
}

PHP_METHOD(MongoCollection, ensureIndex) {
  zval *keys, *options = 0, *db, *system_indexes, *collection, *data, *key_str, *safe_insert = 0;
  mongo_collection *c;

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

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  // get the system.indexes collection
  db = c->parent;

  MAKE_STD_ZVAL(system_indexes);
  ZVAL_STRING(system_indexes, "system.indexes", 1);

  MAKE_STD_ZVAL(collection);
  MONGO_METHOD1(MongoDB, selectCollection, collection, db, system_indexes);

  // set up data
  MAKE_STD_ZVAL(data);
  array_init(data);

  // ns
  add_assoc_zval(data, "ns", c->ns);
  zval_add_ref(&c->ns);
  add_assoc_zval(data, "key", keys);
  zval_add_ref(&keys);

  // turn keys into a string
  MAKE_STD_ZVAL(key_str);

  // MongoCollection::toIndexString()
  MONGO_METHOD1(MongoCollection, toIndexString, key_str, NULL, keys);

  add_assoc_zval(data, "name", key_str);

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
      zval **unique, **drop_dups, **safe;

      // array( "unique" => true )
      if (zend_hash_find(HASH_P(options), "unique", strlen("unique")+1, (void**)&unique) == SUCCESS) {
        add_assoc_zval(data, "unique", *unique);
        zval_add_ref(unique);
      }

      // array( "dropDups" => true )
      if (zend_hash_find(HASH_P(options), "dropDups", strlen("dropDups")+1, (void**)&drop_dups) == SUCCESS) {
        add_assoc_zval(data, "dropDups", *drop_dups);
        zval_add_ref(drop_dups);
      }

      if (zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe) == SUCCESS) {
        if (Z_BVAL_PP(safe)) {
          MAKE_STD_ZVAL(safe_insert);
          ZVAL_BOOL(safe_insert, 1);
        }
      }
    }
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
  zval_ptr_dtor(&key_str);
}

PHP_METHOD(MongoCollection, deleteIndex) {
  zval *keys, *key_str, *data;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &keys) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(key_str);
  MONGO_METHOD1(MongoCollection, toIndexString, key_str, NULL, keys);

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

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
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  MAKE_STD_ZVAL(data);
  array_init(data);

  add_assoc_string(data, "deleteIndexes", Z_STRVAL_P(c->name), 1);
  add_assoc_string(data, "index", "*", 1);

  MONGO_CMD(return_value, c->parent);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, getIndexInfo) {
  zval *collection, *i_str, *query, *cursor, *next;
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  MAKE_STD_ZVAL(collection);

  MAKE_STD_ZVAL(i_str);
  ZVAL_STRING(i_str, "system.indexes", 1);
  MONGO_METHOD1(MongoDB, selectCollection, collection, c->parent, i_str);
  zval_ptr_dtor(&i_str);

  MAKE_STD_ZVAL(query);
  array_init(query);
  add_assoc_string(query, "ns", Z_STRVAL_P(c->ns), 1);

  MAKE_STD_ZVAL(cursor);
  MONGO_METHOD1(MongoCollection, find, cursor, collection, query);

  zval_ptr_dtor(&query);
  zval_ptr_dtor(&collection);

  array_init(return_value);

  MAKE_STD_ZVAL(next);
  MONGO_METHOD(MongoCursor, getNext, next, cursor);
  while (Z_TYPE_P(next) != IS_NULL) {
    add_next_index_zval(return_value, next);

    MAKE_STD_ZVAL(next);
    MONGO_METHOD(MongoCursor, getNext, next, cursor);
  }
  zval_ptr_dtor(&next);
  zval_ptr_dtor(&cursor);
}

PHP_METHOD(MongoCollection, count) {
  zval *response, *data, *query=0;
  zval **n;
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &query) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(response);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "count", Z_STRVAL_P(c->name), 1);
  if (query) {
    add_assoc_zval(data, "query", query);
    zval_add_ref(&query);
  }

  MONGO_CMD(response, c->parent);

  zval_ptr_dtor(&data);
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
  zval *a;
  zval **id;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &a) == FAILURE ||
      IS_SCALAR_P(a)) {
    return;
  }

  if (zend_hash_find(HASH_P(a), "_id", 4, (void**)&id) == SUCCESS) {
    zval zupsert;
    zval *criteria;

    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
    add_assoc_zval(criteria, "_id", *id);
    zval_add_ref(id);

    Z_TYPE(zupsert) = IS_BOOL;
    zupsert.value.lval = 1;

    MONGO_METHOD3(MongoCollection, update, return_value, getThis(), criteria, a, &zupsert);

    zval_ptr_dtor(&criteria);
    return;
  }
  
  MONGO_METHOD1(MongoCollection, insert, return_value, getThis(), a);
}

PHP_METHOD(MongoCollection, createDBRef) {
  zval *obj;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &obj) == FAILURE) {
    return;
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  MONGO_METHOD2(MongoDB, createDBRef, return_value, c->parent, c->name, obj);
}

PHP_METHOD(MongoCollection, getDBRef) {
  zval *ref;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
    return;
  }

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

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
    RETURN_FALSE;
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

        convert_to_long(*data);
        len += Z_LVAL_PP(data) == 1 ? 2 : 3;

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
      
      convert_to_long(*data);
      if (Z_LVAL_PP(data) != 1) {
        *(position)++ = '-';
      }
      *(position)++ = '1';

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
  zval *key, *initial, *condition = 0, *group, *data, *reduce;
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "aaz|z", &key, &initial, &reduce, &condition) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(reduce) == IS_STRING) {
    zval *code;
    MAKE_STD_ZVAL(code);
    object_init_ex(code, mongo_ce_Code);

    MONGO_METHOD1(MongoCode, __construct, return_value, code, reduce);

    reduce = code;
  }
  else if (Z_TYPE_P(reduce) == IS_OBJECT &&
           Z_OBJCE_P(reduce) == mongo_ce_Code) {
    zval_add_ref(&reduce);
  }

  MAKE_STD_ZVAL(data);
  array_init(data);

  MAKE_STD_ZVAL(group);
  array_init(group);
  add_assoc_zval(group, "ns", c->name);
  zval_add_ref(&c->name);
  add_assoc_zval(group, "$reduce", reduce);
  zval_add_ref(&reduce);
  add_assoc_zval(group, "key", key);
  zval_add_ref(&key);
  add_assoc_zval(group, "cond", condition);
  zval_add_ref(&condition);
  add_assoc_zval(group, "initial", initial);
  zval_add_ref(&initial);

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
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
    return;
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

  zend_declare_property_null(mongo_ce_Collection, "db", strlen("db"), ZEND_ACC_PUBLIC TSRMLS_CC);
}
