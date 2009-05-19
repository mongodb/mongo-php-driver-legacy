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

#include "collection.h"
#include "cursor.h"
#include "bson.h"
#include "mongo.h"
#include "mongo_types.h"
#include "db.h"

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_DB,
  *mongo_ce_Cursor,
  *mongo_ce_Exception,
  *spl_ce_InvalidArgumentException;

extern int le_pconnection,
  le_connection,
  le_db_cursor;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_Collection = NULL;

PHP_METHOD(MongoCollection, __construct) {
  zval *db;
  char *name;
  int name_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &db, mongo_ce_DB, &name, &name_len) == FAILURE) {
    return;
  }

  if (strchr(name, '$')) {
    zend_throw_exception(spl_ce_InvalidArgumentException, "MongoCollection->__construct(): collection names cannot contain '$'", 0 TSRMLS_CC);
    return;
  }

  zend_update_property(mongo_ce_Collection, getThis(), "db", strlen("db"), db TSRMLS_CC);
  zend_update_property_stringl(mongo_ce_Collection, getThis(), "name", strlen("name"), name, name_len TSRMLS_CC);

  zval *dbname = zend_read_property(mongo_ce_DB, db, "name", strlen("name"), NOISY TSRMLS_CC);

  char *ns;
  spprintf(&ns, 0, "%s.%s", Z_STRVAL_P(dbname), name);
  zend_update_property_string(mongo_ce_Collection, getThis(), "ns", strlen("ns"), ns TSRMLS_CC);
  efree(ns);

  zval *connection = zend_read_property(mongo_ce_DB, db, "connection", strlen("connection"), NOISY TSRMLS_CC);
  zend_update_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), connection TSRMLS_CC);

  connection = zend_read_property(mongo_ce_DB, db, "name", strlen("name"), NOISY TSRMLS_CC);
}

PHP_METHOD(MongoCollection, __toString) {
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), NOISY TSRMLS_CC);
  RETURN_STRING(Z_STRVAL_P(ns), 1);
}

PHP_METHOD(MongoCollection, getName) {
  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);
  RETURN_ZVAL(name, 1, 1);
}

PHP_METHOD(MongoCollection, drop) {
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  zval *name = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);
  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);
  db = zend_read_property(mongo_ce_Collection, db, "name", strlen("name"), NOISY TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "drop", Z_STRVAL_P(name), 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, db, 3, NULL);

  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, validate) {
  zend_bool scan_data = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &scan_data) == FAILURE) {
    return;
  }

  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);
  zval *zlink = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);

  zval_add_ref(&name);
  add_assoc_zval(data, "validate", name);

  if (scan_data) {
    add_assoc_bool(data, "scandata", scan_data);
  }

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);
  db = zend_read_property(mongo_ce_DB, db, "name", strlen("name"), NOISY TSRMLS_CC);

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, db, 3, NULL);
  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, insert) {
  zval *a;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &a) == FAILURE) {
    return;
  }

  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);
  int response = mongo_do_insert(link, Z_STRVAL_P(ns), a TSRMLS_CC);
  RETURN_BOOL(response >= SUCCESS);
}

PHP_METHOD(MongoCollection, batchInsert) {
  zval *a;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &a) == FAILURE) {
    return;
  }

  mongo_link *link;
  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &connection, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(ns), Z_STRLEN_P(ns), OP_INSERT);

  HashTable *php_array = Z_ARRVAL_P(a);

  int count = 0;
  zval **data;
  HashPosition pointer;
  for(zend_hash_internal_pointer_reset_ex(php_array, &pointer); 
      zend_hash_get_current_data_ex(php_array, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(php_array, &pointer)) {

    if(Z_TYPE_PP(data) != IS_ARRAY) {
      efree(buf.start);
      RETURN_FALSE;
    }

    unsigned int start = buf.pos-buf.start;
    zval_to_bson(&buf, Z_ARRVAL_PP(data), NO_PREP TSRMLS_CC);

    serialize_size(buf.start+start, &buf);

    count++;
  }

  // if there are no elements, don't bother saving
  if (count == 0) {
    efree(buf.start);
    RETURN_FALSE;
  }

  serialize_size(buf.start, &buf);

  RETVAL_BOOL(mongo_say(link, &buf TSRMLS_CC)+1);
  efree(buf.start);
}

PHP_METHOD(MongoCollection, find) {
  void *holder;
  zval *query = 0, *fields = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|aa", &query, &fields) == FAILURE) {
    return;
  }

  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), NOISY TSRMLS_CC);

  object_init_ex(return_value, mongo_ce_Cursor);

  zend_ptr_stack_2_push(&EG(argument_stack), connection, ns);

  if (query) {
    zend_ptr_stack_push(&EG(argument_stack), query);
    if (fields) {
      zend_ptr_stack_push(&EG(argument_stack), fields);
    }
  }
  
  zend_ptr_stack_2_push(&EG(argument_stack), (void*)ZEND_NUM_ARGS()+2, NULL);
  
  zval temp;
  zim_MongoCursor___construct(ZEND_NUM_ARGS()+2, &temp, NULL, return_value, return_value_used TSRMLS_CC);

  zend_ptr_stack_n_pop(&EG(argument_stack), ZEND_NUM_ARGS()+4, &holder, &holder, &holder, &holder, &holder, &holder);
}

PHP_METHOD(MongoCollection, findOne) {
  zval *query = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &query) == FAILURE) {
    return;
  }

  zval *cursor;
  MAKE_STD_ZVAL(cursor);

  if (query) {
    zend_ptr_stack_n_push(&EG(argument_stack), 3, query, 1, NULL);
  }
  zim_MongoCollection_find(ZEND_NUM_ARGS(), cursor, &cursor, getThis(), return_value_used TSRMLS_CC);

  void *holder;
  if (query) {
    zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);
  }

  zval limit;
  limit.type = IS_LONG;
  limit.value.lval = 1;

  zend_ptr_stack_n_push(&EG(argument_stack), 3, &limit, 1, NULL);
  zim_MongoCursor_limit(1, cursor, &cursor, cursor, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zim_MongoCursor_getNext(0, return_value, return_value_ptr, cursor, return_value_used TSRMLS_CC);

  zend_objects_store_del_ref(cursor TSRMLS_CC);
  zval_ptr_dtor(&cursor);
}

PHP_METHOD(MongoCollection, update) {
  zval *criteria, *newobj;
  zend_bool upsert = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "aa|b", &criteria, &newobj, &upsert) == FAILURE) {
    return;
  }

  mongo_link *link;
  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &connection, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(ns), Z_STRLEN_P(ns), OP_UPDATE);
  serialize_int(&buf, upsert);
  zval_to_bson(&buf, Z_ARRVAL_P(criteria), NO_PREP TSRMLS_CC);
  zval_to_bson(&buf, Z_ARRVAL_P(newobj), NO_PREP TSRMLS_CC);
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(mongo_say(link, &buf TSRMLS_CC)+1);
  efree(buf.start);
}

PHP_METHOD(MongoCollection, remove) {
  zval *criteria = 0;
  zend_bool just_one = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ab", &criteria, &just_one) == FAILURE) {
    return;
  }

  if (!criteria) {
    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
  }
  else {
    zval_add_ref(&criteria);
  }

  mongo_link *link;
  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &connection, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), NOISY TSRMLS_CC);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(ns), Z_STRLEN_P(ns), OP_DELETE);

  int mflags = (just_one == 1);

  serialize_int(&buf, mflags);
  zval_to_bson(&buf, Z_ARRVAL_P(criteria), NO_PREP TSRMLS_CC);
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(mongo_say(link, &buf TSRMLS_CC)+1);

  efree(buf.start);
  zval_ptr_dtor(&criteria);
}

PHP_METHOD(MongoCollection, ensureIndex) {
  zval *keys;
  zend_bool unique;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &keys, &unique) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(keys) != IS_ARRAY) {
    convert_to_string(keys);

    zval *key_array;
    MAKE_STD_ZVAL(key_array);
    array_init(key_array);
    add_assoc_long(key_array, Z_STRVAL_P(keys), 1);

    keys = key_array;
  }
  else {
    zval_add_ref(&keys);
  }

  // get the system.indexes collection
  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);

  zval *system_indexes;
  MAKE_STD_ZVAL(system_indexes);
  ZVAL_STRING(system_indexes, "system.indexes", 1);

  zval *collection;
  MAKE_STD_ZVAL(collection);

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 3, system_indexes, 1, NULL);
  zim_MongoDB_selectCollection(1, collection, &collection, db, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  // set up data
  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);

  // ns
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), NOISY TSRMLS_CC);
  zval_add_ref(&ns);
  add_assoc_zval(data, "ns", ns);
  add_assoc_zval(data, "key", keys);

  // turn keys into a string
  zval *key_str;
  MAKE_STD_ZVAL(key_str);

  // MongoUtil::toIndexString()
  zend_ptr_stack_n_push(&EG(argument_stack), 3, keys, 1, NULL);
  zim_MongoUtil_toIndexString(1, key_str, &key_str, NULL, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  add_assoc_zval(data, "name", key_str);
  add_assoc_bool(data, "unique", unique);

  // MongoCollection::insert()
  zend_ptr_stack_n_push(&EG(argument_stack), 3, data, 1, NULL);
  zim_MongoCollection_insert(1, return_value, return_value_ptr, collection, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval_ptr_dtor(&data); 
  zval_ptr_dtor(&system_indexes);
  zval_ptr_dtor(&collection);
  zval_ptr_dtor(&keys);
}

PHP_METHOD(MongoCollection, deleteIndex) {
  zval *keys;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &keys) == FAILURE) {
    return;
  }

  void *holder;
  zval *key_str;
  MAKE_STD_ZVAL(key_str);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, keys, 1, NULL);
  zim_MongoUtil_toIndexString(1, key_str, &key_str, NULL, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);
  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);
  zval *db_name = zend_read_property(mongo_ce_DB, db, "name", strlen("name"), NOISY TSRMLS_CC);
  zval *connection = zend_read_property(mongo_ce_DB, db, "connection", strlen("connection"), NOISY TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  zval_add_ref(&name);
  add_assoc_zval(data, "deleteIndexes", name);
  zval_add_ref(&key_str);
  add_assoc_zval(data, "index", key_str);

  zend_ptr_stack_n_push(&EG(argument_stack), 5, connection, data, db_name, 3, NULL);
  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&data);
  zval_ptr_dtor(&key_str);
}

PHP_METHOD(MongoCollection, deleteIndexes) {
  zval *zlink = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);

  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), 1 TSRMLS_CC);
  zval_add_ref(&name);
  add_assoc_zval(data, "deleteIndexes", name);
  add_assoc_string(data, "index", "*", 1);

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), 0 TSRMLS_CC);
  db = zend_read_property(mongo_ce_Collection, db, "name", strlen("name"), 0 TSRMLS_CC);

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, db, 3, NULL);

  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoCollection, getIndexInfo) {
  zval *collection;
  MAKE_STD_ZVAL(collection);

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), NOISY TSRMLS_CC);

  zval *i_str;
  MAKE_STD_ZVAL(i_str);
  ZVAL_STRING(i_str, "system.indexes", 1);

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 3, i_str, 1, NULL);
  zim_MongoDB_selectCollection(1, collection, &collection, db, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval *query;
  MAKE_STD_ZVAL(query);
  array_init(query);
  zval_add_ref(&ns);
  add_assoc_zval(query, "ns", ns);

  zval *cursor;
  MAKE_STD_ZVAL(cursor);
  zend_ptr_stack_n_push(&EG(argument_stack), 3, query, 1, NULL);
  zim_MongoCollection_find(1, cursor, &cursor, collection, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  array_init(return_value);

  zval *has_next;
  MAKE_STD_ZVAL(has_next);
  zim_MongoCursor_hasNext(0, has_next, &has_next, cursor, return_value_used TSRMLS_CC);
  while (Z_BVAL_P(has_next)) {
    zval *next;
    MAKE_STD_ZVAL(next);
    zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);

    add_next_index_zval(return_value, next);

    zim_MongoCursor_hasNext(0, has_next, &has_next, cursor, return_value_used TSRMLS_CC);
  }
}

PHP_METHOD(MongoCollection, count) {
  zval *zlink = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);
  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);
  zval *db_name = zend_read_property(mongo_ce_DB, db, "name", strlen("name"), NOISY TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "count", Z_STRVAL_P(name), 1);

  zval *response;
  MAKE_STD_ZVAL(response);

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, db_name, 3, NULL);
  zim_MongoUtil_dbCommand(3, response, &response, NULL, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&data);
  zval **n;
  if (zend_hash_find(Z_ARRVAL_P(response), "n", 2, (void**)&n) == SUCCESS) {
    RETURN_ZVAL(*n, 1, 1);
  }
  else {
    RETURN_ZVAL(response, 0, 1);
  }
}

PHP_METHOD(MongoCollection, save) {
  zval *a;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &a) == FAILURE) {
    return;
  }

  zval *holder;
  zval **id;
  if (zend_hash_find(Z_ARRVAL_P(a), "_id", 4, (void**)&id) == SUCCESS) {
    zval *criteria;
    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
    add_assoc_zval(criteria, "_id", *id);

    zval zupsert;
    Z_TYPE(zupsert) = IS_BOOL;
    zupsert.value.lval = 1;

    zend_ptr_stack_n_push(&EG(argument_stack), 5, criteria, a, &zupsert, 3, NULL);
    zim_MongoCollection_update(3, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);

    zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);
    return;
  }
  
  zend_ptr_stack_n_push(&EG(argument_stack), 3, a, 1, NULL);
  zim_MongoCollection_insert(1, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);
}

PHP_METHOD(MongoCollection, createDBRef) {
  zval *obj;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &obj) == FAILURE) {
    return;
  }

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);
  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 4, name, obj, 2, NULL);
  zim_MongoDB_createDBRef(2, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
}

PHP_METHOD(MongoCollection, getDBRef) {
  zval *ref;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
    return;
  }

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 3, ref, 1, NULL);
  zim_MongoDB_getDBRef(1, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);
}

static function_entry MongoCollection_methods[] = {
  PHP_ME(MongoCollection, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCollection, __toString, NULL, ZEND_ACC_PUBLIC)
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
  {NULL, NULL, NULL}
};

void mongo_init_MongoCollection(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoCollection", MongoCollection_methods);
  mongo_ce_Collection = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_null(mongo_ce_Collection, "db", strlen("db"), ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Collection, "name", strlen("name"), ZEND_ACC_PUBLIC TSRMLS_CC);
}
