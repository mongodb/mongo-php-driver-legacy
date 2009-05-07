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

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_DB,
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
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Cs", &db, &mongo_ce_DB, &name, &name_len) == FAILURE) {
    return;
  }

  zend_update_property(mongo_ce_Collection, getThis(), "db", strlen("db"), db TSRMLS_CC);
  if (name_len == 0 ||
      strchr(name, '$')) {
    zend_throw_exception(spl_ce_InvalidArgumentException, "MongoCollection->__construct(): collection names must be at least one character and cannot contain '$'", 0 TSRMLS_CC);
    return;
  }

  zend_update_property_string(mongo_ce_Collection, getThis(), "name", strlen("name"), name TSRMLS_CC);

  zval *ns;
  zim_MongoCollection___toString(ht, ns, &ns, getThis(), 0 TSRMLS_CC);
  zend_update_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), ns TSRMLS_CC);

  zval *connection = zend_read_property(mongo_ce_DB, db, "connection", strlen("connection"), NOISY TSRMLS_CC);
  zend_update_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), connection TSRMLS_CC);
}

PHP_METHOD(MongoCollection, __toString) {
  zval *db_r;
  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), 1 TSRMLS_CC);
  zim_MongoDB___toString(ht, db_r, &db_r, db, 0 TSRMLS_CC);

  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), 1 TSRMLS_CC);

  char *ns;
  spprintf(&ns, 0, "%s.%s", Z_STRVAL_P(db_r), Z_STRVAL_P(name));
  RETURN_STRING(ns, 0);
}

PHP_METHOD(MongoCollection, getName) {
  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), 1 TSRMLS_CC);
  RETURN_ZVAL(name, 0, 0);
}

PHP_METHOD(MongoCollection, drop) {
  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *ns = zend_read_property(mongo_ce_Mongo, getThis(), "ns", strlen("ns"), 0 TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "drop", Z_STRVAL_P(ns), 0);

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), 0 TSRMLS_CC);
  db = zend_read_property(mongo_ce_Collection, db, "name", strlen("name"), 0 TSRMLS_CC);

  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(db));
}

PHP_METHOD(MongoCollection, validate) {
  zend_bool scan_data = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &scan_data) == FAILURE) {
    return;
  }

  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), 1 TSRMLS_CC);
  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  zval *cmd;
  MAKE_STD_ZVAL(cmd);
  array_init(cmd);
  add_assoc_zval(cmd, "validate", name);
  if (scan_data) {
    add_assoc_bool(cmd, "scandata", scan_data);
  }

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), 0 TSRMLS_CC);
  db = zend_read_property(mongo_ce_Collection, db, "name", strlen("name"), 0 TSRMLS_CC);

  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, cmd, Z_STRVAL_P(db));
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

  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
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

  RETVAL_BOOL(say(connection, &buf TSRMLS_CC)+1);
  efree(buf.start);
}

PHP_METHOD(MongoCollection, find) {
  zval *query, *fields;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|aa", &query, fields) == FAILURE) {
    return;
  }

  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  zval *z;
  MAKE_STD_ZVAL(z);
  object_init(z);

  //TODO: create cursor

  RETURN_ZVAL(z, 0, 1);
}

PHP_METHOD(MongoCollection, findOne) {
  zval *query;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &query) == FAILURE) {
    return;
  }

  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  mongo_cursor *cursor = mongo_do_query(link, Z_STRVAL_P(ns), 0, -1, query, 0 TSRMLS_CC);
  if (!cursor || !mongo_do_has_next(cursor TSRMLS_CC)) {
    RETURN_NULL();
  }

  zval *next = mongo_do_next(cursor TSRMLS_CC);
  RETURN_ZVAL(next, 0, 1);
}

PHP_METHOD(MongoCollection, update) {
  zval *criteria, *newobj;
  zend_bool upsert;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "aa|!b", &criteria, &newobj, &upsert) == FAILURE) {
    return;
  }

  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, Z_STRVAL_P(ns), Z_STRLEN_P(ns), OP_UPDATE);
  serialize_int(&buf, upsert);
  zval_to_bson(&buf, Z_ARRVAL_P(criteria), NO_PREP TSRMLS_CC);
  zval_to_bson(&buf, Z_ARRVAL_P(newobj), NO_PREP TSRMLS_CC);
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(say(connection, &buf TSRMLS_CC)+1);
  efree(buf.start);
}

PHP_METHOD(MongoCollection, remove) {
  zval *criteria;
  zend_bool just_one;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ab", &criteria, &just_one) == FAILURE) {
    return;
  }

  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);

  HashTable *array = Z_ARRVAL_P(criteria);
  CREATE_HEADER(buf, Z_STRVAL_P(ns), Z_STRLEN_P(ns), OP_DELETE);

  int mflags = (just_one == 1);

  serialize_int(&buf, mflags);
  zval_to_bson(&buf, Z_ARRVAL_P(criteria), NO_PREP TSRMLS_CC);
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(say(connection, &buf TSRMLS_CC)+1);
  efree(buf.start);
}

PHP_METHOD(MongoCollection, ensureIndex) {
  zval *keys;
  zend_bool unique;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|b", &keys, &unique) == FAILURE) {
    return;
  }

  zval *connection = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  zval *ns = zend_read_property(mongo_ce_Collection, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);

  // get the system.indexes collection
  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), 1 TSRMLS_CC);

  zval *system_indexes;
  MAKE_STD_ZVAL(system_indexes);
  ZVAL_STRING(system_indexes, "system.indexes", 1);

}

PHP_METHOD(MongoCollection, deleteIndex) {

}

PHP_METHOD(MongoCollection, deleteIndexes) {
  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);

  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), 1 TSRMLS_CC);
  add_assoc_zval(data, "deleteIndexes", name);
  add_assoc_string(data, "index", "*", 0);

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), 0 TSRMLS_CC);
  db = zend_read_property(mongo_ce_Collection, db, "name", strlen("name"), 0 TSRMLS_CC);

  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(db));
}

PHP_METHOD(MongoCollection, getIndexInfo) {
  //TODO
}

PHP_METHOD(MongoCollection, count) {
  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Collection, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);

  zval *name = zend_read_property(mongo_ce_Collection, getThis(), "name", strlen("name"), 1 TSRMLS_CC);
  add_assoc_zval(data, "count", name);

  zval *db = zend_read_property(mongo_ce_Collection, getThis(), "db", strlen("db"), 0 TSRMLS_CC);
  db = zend_read_property(mongo_ce_Collection, db, "name", strlen("name"), 0 TSRMLS_CC);

  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(db));

  zval **n;
  if (zend_hash_find(Z_ARRVAL_P(return_value), "n", 2, (void**)n) == SUCCESS) {
    RETURN_LONG(Z_LVAL_PP(n));
  }
  RETURN_LONG(-1);
}

PHP_METHOD(MongoCollection, save) {
  zval *a;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &a) == FAILURE) {
    return;
  }

  int null_ptr = 0, upsert = 1;
  zval **id;
  if (zend_hash_find(Z_ARRVAL_P(return_value), "_id", 4, (void**)id) == SUCCESS) {
    zval *criteria;
    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
    add_assoc_zval(criteria, "_id", *id);

    zend_ptr_stack_n_push(&EG(argument_stack), 5, criteria, a, &upsert, 3, &null_ptr);
    zim_MongoCollection_update(INTERNAL_FUNCTION_PARAM_PASSTHRU);

    zval *holder;
    zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);
    return;
  }

  zend_ptr_stack_n_push(&EG(argument_stack), 3, a, 1, &null_ptr);
  zim_MongoCollection_update(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  
  zval *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);
}

PHP_METHOD(MongoCollection, createDBRef) {
  //TODO
}

PHP_METHOD(MongoCollection, getDBRef) {
  //TODO
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
