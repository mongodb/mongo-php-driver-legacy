// db.c
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

#include "db.h"
#include "mongo.h"

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_Collection,
  *mongo_ce_Id,
  *spl_ce_InvalidArgumentException;

extern int le_pconnection,
  le_connection;

zend_class_entry *mongo_ce_DB = NULL;

PHP_METHOD(MongoDB, __construct) {
  zval *zlink;
  char *name;
  int name_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Cs", &zlink, &mongo_ce_Mongo, &name, &name_len) == FAILURE) {
    return;
  }

  if (name_len == 0 ||
      strchr(name, ' ') ||
      strchr(name, '.')) {
    zend_throw_exception(spl_ce_InvalidArgumentException, "MongoDB->__construct(): database names must be at least one character and cannot contain ' ' or  '.'", 0 TSRMLS_CC);
    return;
  }

  zval *zconn = zend_read_property(mongo_ce_Mongo, zlink, "connection", strlen("connection"), 0 TSRMLS_CC);
  zend_update_property(mongo_ce_DB, getThis(), "connection", strlen("connection"), zconn TSRMLS_CC);
  zend_update_property_string(mongo_ce_DB, getThis(), "name", strlen("name"), name TSRMLS_CC);
}

PHP_METHOD(MongoDB, __toString) {
  zval *name = zend_read_property(mongo_ce_DB, getThis(), "name", strlen("name"), 0 TSRMLS_CC);
  RETURN_STRING(Z_STRVAL_P(name), 0);
}

PHP_METHOD(MongoDB, selectCollection) {
  //TODO!
}

PHP_METHOD(MongoDB, getGridFS) {
  //TODO
}

PHP_METHOD(MongoDB, getProfilingLevel) {
  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "profile", -1);

  zval **ok;
  if (zend_hash_find(Z_ARRVAL_P(return_value), "ok", 3, (void**)&ok) == SUCCESS &&
      Z_LVAL_P(return_value) == 1) {
    zend_hash_find(Z_ARRVAL_P(return_value), "was", 4, (void**)&ok);
    RETURN_ZVAL(*ok, 0, 1);
  }
  RETURN_FALSE;
}

PHP_METHOD(MongoDB, setProfilingLevel) {
  long level;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &level) == FAILURE) {
    return;
  }

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "profile", level);

  zval **ok;
  if (zend_hash_find(Z_ARRVAL_P(return_value), "ok", 3, (void**)&ok) == SUCCESS &&
      Z_LVAL_P(return_value) == 1) {
    zend_hash_find(Z_ARRVAL_P(return_value), "was", 4, (void**)&ok);
    RETURN_ZVAL(*ok, 0, 1);
  }
  RETURN_FALSE;
}

PHP_METHOD(MongoDB, drop) {
  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "dropDatabase", 1);

  zval *name = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), 0 TSRMLS_CC);

  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(name));
}

PHP_METHOD(MongoDB, repair) {
  zend_bool cloned=0, original=0;
  zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|bb", &cloned, &original);

  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "repairDatabase", 1);
  add_assoc_bool(data, "preserveClonedFilesOnFailure", cloned);
  add_assoc_bool(data, "backupOriginalFiles", original);

  zval *name = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), 0 TSRMLS_CC);

  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(name));
}


PHP_METHOD(MongoDB, createCollection) {
  char *cname;
  int cname_len;
  zend_bool capped=0;
  int size=0, max=0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|bll", &cname, &cname_len, &capped, &size, &max) == FAILURE) {
    return;
  }

  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_DB, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "create", cname, 0);
  if (capped && size) {
    add_assoc_bool(data, "capped", 1);
    add_assoc_long(data, "size", size);
    if (max) {
      add_assoc_long(data, "max", max);
    }
  }

  zval *name = zend_read_property(mongo_ce_DB, getThis(), "name", strlen("name"), 0 TSRMLS_CC);

  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(name));

  int null_ptr = 0;
  zend_ptr_stack_n_push(&EG(argument_stack), 4, getThis(), cname, 2, &null_ptr);
  zim_MongoDB_selectCollection(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  zval *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
}

PHP_METHOD(MongoDB, dropCollection) {
  zval *coll;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &coll) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(coll) == IS_OBJECT &&
      Z_OBJCE_P(coll) == mongo_ce_Collection) {
    zim_MongoCollection_drop(0, return_value, return_value_ptr, coll, return_value_used TSRMLS_CC);
  }
  else {
    int null_ptr = 0;
    zend_ptr_stack_n_push(&EG(argument_stack), 4, getThis(), coll, 2, &null_ptr);
    zim_MongoDB_selectCollection(1, coll, &coll, getThis(), return_value_used TSRMLS_CC);
    zval *holder;
    zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);

    zim_MongoCollection_drop(0, return_value, return_value_ptr, coll, return_value_used TSRMLS_CC);
  }
}

PHP_METHOD(MongoDB, listCollections) {
  char *name;
  zval *zname = zend_read_property(mongo_ce_DB, getThis(), "name", strlen("name"), 0 TSRMLS_CC);
  int name_len = spprintf(&name, 0, "%s.system.indexes", Z_STRVAL_P(zname));

  zval *zlink = zend_read_property(mongo_ce_DB, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  mongo_link *link;
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *list;
  MAKE_STD_ZVAL(list);
  array_init(list);

  mongo_cursor *cursor = mongo_do_query(link, name, 0, 0, NULL, NULL TSRMLS_CC);
  while (mongo_do_has_next(cursor TSRMLS_CC)) {

    zval *obj = mongo_do_next(cursor TSRMLS_CC);

    zval *cname;
    MAKE_STD_ZVAL(cname);
    zval **ccname = &cname;

    if (zend_hash_find(Z_ARRVAL_P(obj), "name", 5, (void**)&ccname) == FAILURE ||
        strchr(Z_STRVAL_P(cname), '$')) {
      continue;
    }

    add_next_index_zval(list, cname);

    zval_add_ref(ccname);
    zval_ptr_dtor(&obj);
  }

  efree(name);

  RETURN_ZVAL(list, 0, 1);
}

PHP_METHOD(MongoDB, getCursorInfo) {
  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "cursorInfo", 1);

  zval *zname = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), 0 TSRMLS_CC);
  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(zname));
}

PHP_METHOD(MongoDB, createDBRef) {
  zval *ns, *obj, *holder;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &ns, &obj) == FAILURE) {
    return;
  }

  convert_to_string(ns);

  zval *ref;
  MAKE_STD_ZVAL(ref);
  array_init(ref);
  add_assoc_string(ref, "$ref", Z_STRVAL_P(ns), 1);

  zval **id;
  if (Z_TYPE_P(obj) == IS_ARRAY &&
      zend_hash_find(Z_ARRVAL_P(obj), "_id", 4, (void**)id) == SUCCESS) {

    add_assoc_zval(ref, "$id", *id);
  }
  else if (Z_TYPE_P(obj) == IS_OBJECT &&
           Z_OBJCE_P(obj) == mongo_ce_Id) {
    add_assoc_zval(ref, "$id", obj);
  }

  RETURN_ZVAL(ref, 0, 1);
}

PHP_METHOD(MongoDB, getDBRef) {
  //TODO
}

PHP_METHOD(MongoDB, execute) {
  zval *code, *args;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &code, &args) == FAILURE) {
    return;
  }
  if (!args) {
    MAKE_STD_ZVAL(args);
    array_init(args);
  }

  if (Z_TYPE_P(code) != IS_OBJECT || 
      Z_OBJCE_P(code) != mongo_ce_Id) {
    //TODO
  }

  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_zval(data, "$eval", code);
  add_assoc_zval(data, "$args", args);

  zval *zname = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), 0 TSRMLS_CC);
  mongo_db_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, link, data, Z_STRVAL_P(zname));
}

static function_entry MongoDB_methods[] = {
  PHP_ME(MongoDB, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, __toString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getGridFS, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getProfilingLevel, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, setProfilingLevel, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, drop, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, repair, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, selectCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, createCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, dropCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, listCollections, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getCursorInfo, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, createDBRef, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getDBRef, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, execute, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoDB(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoDB", MongoDB_methods);
  mongo_ce_DB = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_OFF", strlen("PROFILING_OFF"), 0 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_SLOW", strlen("PROFILING_SLOW"), 1 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_ON", strlen("PROFILING_ON"), 2 TSRMLS_CC);

  zend_declare_property_null(mongo_ce_DB, "connection", strlen("connection"), ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_ce_DB, "name", strlen("cursor"), ZEND_ACC_PUBLIC TSRMLS_CC);
}
