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
  *mongo_ce_GridFS,
  *mongo_ce_Id,
  *mongo_ce_Code,
  *spl_ce_InvalidArgumentException;

extern int le_pconnection,
  le_connection;

zend_class_entry *mongo_ce_DB = NULL;

PHP_METHOD(MongoDB, __construct) {
  zval *zlink;
  char *name;
  int name_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &zlink, mongo_ce_Mongo, &name, &name_len) == FAILURE) {
    return;
  }

  if (name_len == 0 ||
      strchr(name, ' ') ||
      strchr(name, '.')) {
    zend_throw_exception(spl_ce_InvalidArgumentException, "MongoDB->__construct(): database names must be at least one character and cannot contain ' ' or  '.'", 0 TSRMLS_CC);
    return;
  }

  zval *zconn = zend_read_property(mongo_ce_Mongo, zlink, "connection", strlen("connection"), NOISY TSRMLS_CC);
  zend_update_property(mongo_ce_DB, getThis(), "connection", strlen("connection"), zconn TSRMLS_CC);
  zend_update_property_stringl(mongo_ce_DB, getThis(), "name", strlen("name"), name, name_len TSRMLS_CC);
}

PHP_METHOD(MongoDB, __toString) {
  zval *name = zend_read_property(mongo_ce_DB, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);
  RETURN_STRING(Z_STRVAL_P(name), 1);
}

PHP_METHOD(MongoDB, selectCollection) {
  zval *collection;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &collection) == FAILURE) {
    return;
  }

  zval *obj;
  MAKE_STD_ZVAL(obj);
  object_init_ex(obj, mongo_ce_Collection);

  zend_ptr_stack_n_push(&EG(argument_stack), 4, getThis(), collection, 2, NULL);
  zim_MongoCollection___construct(2, return_value, return_value_ptr, obj, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
  RETURN_ZVAL(obj, 0, 1);
}

PHP_METHOD(MongoDB, getGridFS) {
  zval *arg1 = 0, *arg2 = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &arg1, &arg2) == FAILURE) {
    return;
  }

  zval temp;
  void *holder;

  object_init_ex(return_value, mongo_ce_GridFS);
  
  zend_ptr_stack_push(&EG(argument_stack), getThis());

  if (arg1) {
    zend_ptr_stack_push(&EG(argument_stack), arg1);
    if (arg2) {
      zend_ptr_stack_push(&EG(argument_stack), arg2);
    }
  }

  zend_ptr_stack_2_push(&EG(argument_stack), ZEND_NUM_ARGS()+1, NULL);
  zim_MongoGridFS___construct(ZEND_NUM_ARGS()+1, &temp, NULL, return_value, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  if (arg1) {
    zend_ptr_stack_pop(&EG(argument_stack));
    if (arg2) {
      zend_ptr_stack_pop(&EG(argument_stack));
    }
  }
}

PHP_METHOD(MongoDB, getProfilingLevel) {
  zval l;
  Z_TYPE(l) = IS_LONG;
  Z_LVAL(l) = -1;

  zend_ptr_stack_n_push(&EG(argument_stack), 3, &l, 1, NULL);
  zim_MongoDB_setProfilingLevel(1, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);
}

PHP_METHOD(MongoDB, setProfilingLevel) {
  long level;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &level) == FAILURE) {
    return;
  }

  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "profile", level);

  zval *name = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);

  zval *cmd_return, *cmd_ptr;
  MAKE_STD_ZVAL(cmd_return);
  cmd_ptr = cmd_return;

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, name, 3, NULL);
  zim_MongoUtil_dbCommand(3, cmd_return, &cmd_return, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);
  zval_ptr_dtor(&data);

  zval **ok;
  if (zend_hash_find(Z_ARRVAL_P(cmd_return), "ok", 3, (void**)&ok) == SUCCESS &&
      Z_DVAL_PP(ok) == 1) {
    zend_hash_find(Z_ARRVAL_P(cmd_return), "was", 4, (void**)&ok);
    RETVAL_ZVAL(*ok, 1, 1);
  }
  else {
    RETVAL_NULL();
  }
  zval_ptr_dtor(&cmd_return);
  zval_ptr_dtor(&cmd_ptr);
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

  zval *cmd_return, *cmd_ptr;
  MAKE_STD_ZVAL(cmd_return);
  cmd_ptr = cmd_return;

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, name, 3, NULL);
  zim_MongoUtil_dbCommand(3, cmd_return, &cmd_return, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);
  zval_ptr_dtor(&data);
  zval_ptr_dtor(&cmd_return);
  zval_ptr_dtor(&cmd_ptr);
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

  zval *cmd_return, *cmd_ptr;
  MAKE_STD_ZVAL(cmd_return);
  cmd_ptr = cmd_return;

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, name, 3, NULL);
  zim_MongoUtil_dbCommand(3, cmd_return, &cmd_return, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);
  zval_ptr_dtor(&data);
  zval_ptr_dtor(&cmd_return);
  zval_ptr_dtor(&cmd_ptr);
}


PHP_METHOD(MongoDB, createCollection) {
  zval *collection;
  zend_bool capped=0;
  int size=0, max=0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|bll", &collection, &capped, &size, &max) == FAILURE) {
    return;
  }
  convert_to_string(collection);

  zval *zlink = zend_read_property(mongo_ce_DB, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  zval *name = zend_read_property(mongo_ce_DB, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_zval(data, "create", collection);
  if (capped && size) {
    add_assoc_bool(data, "capped", 1);
    add_assoc_long(data, "size", size);
    if (max) {
      add_assoc_long(data, "max", max);
    }
  }

  zval *cmd_return, *cmd_ptr;
  MAKE_STD_ZVAL(cmd_return);
  cmd_ptr = cmd_return;

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, name, 3, NULL);
  zim_MongoUtil_dbCommand(3, cmd_return, &cmd_return, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);

  // get the collection we just created
  zend_ptr_stack_n_push(&EG(argument_stack), 3, collection, 1, NULL);
  zim_MongoDB_selectCollection(1, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval_ptr_dtor(&data);
  zval_ptr_dtor(&cmd_ptr);
}

PHP_METHOD(MongoDB, dropCollection) {
  zval *collection;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &collection) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(collection) != IS_OBJECT ||
      Z_OBJCE_P(collection) != mongo_ce_Collection) {
    zend_ptr_stack_n_push(&EG(argument_stack), 4, getThis(), collection, 2, NULL);
    zim_MongoDB_selectCollection(1, collection, &collection, getThis(), return_value_used TSRMLS_CC);

    void *holder;
    zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
  }
  zim_MongoCollection_drop(0, return_value, return_value_ptr, collection, return_value_used TSRMLS_CC);
}

PHP_METHOD(MongoDB, listCollections) {
  // select db.system.namespaces collection
  zval *nss;
  MAKE_STD_ZVAL(nss);
  ZVAL_STRING(nss, "system.namespaces", 1);

  zval *collection;
  MAKE_STD_ZVAL(collection);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, nss, 1, NULL);
  zim_MongoDB_selectCollection(1, collection, &collection, getThis(), return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);
  
  // do find  
  zval *cursor;
  MAKE_STD_ZVAL(cursor);
  zim_MongoCollection_find(0, cursor, &cursor, collection, return_value_used TSRMLS_CC);

  // list to return
  zval *list;
  MAKE_STD_ZVAL(list);
  array_init(list);
 
  // populate list
  zval *next;
  MAKE_STD_ZVAL(next);
  
  zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);
  while (Z_TYPE_P(next) != IS_NULL) {
    zval **collection;
    if (zend_hash_find(Z_ARRVAL_P(next), "name", 5, (void**)&collection) == FAILURE ||
        strchr(Z_STRVAL_PP(collection), '$')) {

      zval_ptr_dtor(&next);
      MAKE_STD_ZVAL(next);

      zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);
      continue;
    }

    add_next_index_string(list, Z_STRVAL_PP(collection), 1);

    zval_ptr_dtor(&next);
    MAKE_STD_ZVAL(next);

    zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);
  }

  zval_ptr_dtor(&next);
  zval_ptr_dtor(&nss);
  zval_ptr_dtor(&cursor);
  zval_ptr_dtor(&collection);

  RETURN_ZVAL(list, 0, 1);
}

PHP_METHOD(MongoDB, getCursorInfo) {
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "cursorInfo", 1);

  zval *zname = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), 0 TSRMLS_CC);

  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, data, zname, 3, NULL);

  zim_MongoUtil_dbCommand(3, return_value, &return_value, getThis(), return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);
  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoDB, createDBRef) {
  zval *ns, *obj;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &ns, &obj) == FAILURE) {
    return;
  }

  void *holder;
  zval **id;
  if (Z_TYPE_P(obj) == IS_ARRAY &&
      zend_hash_find(Z_ARRVAL_P(obj), "_id", 4, (void**)&id) == SUCCESS) {

    zval_add_ref(&ns);
    zval_add_ref(id);
    zend_ptr_stack_n_push(&EG(argument_stack), 4, ns, *id, 2, NULL);
    zim_MongoDBRef_create(2, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);
    zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);

    return;
  }
  else if (Z_TYPE_P(obj) == IS_OBJECT &&
           Z_OBJCE_P(obj) == mongo_ce_Id) {

    zval_add_ref(&ns);
    zval_add_ref(&obj);
    zend_ptr_stack_n_push(&EG(argument_stack), 4, ns, obj, 2, NULL);
    zim_MongoDBRef_create(2, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);
    zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);

    return;
  }
  RETURN_NULL();
}

PHP_METHOD(MongoDB, getDBRef) {
  zval *ref;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
    return;
  }

  zend_ptr_stack_n_push(&EG(argument_stack), 4, getThis(), ref, 2, NULL);

  zim_MongoDBRef_get(2, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
}

PHP_METHOD(MongoDB, execute) {
  zval *code = 0, *args = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &code, &args) == FAILURE) {
    return;
  }

  if (!args) {
    MAKE_STD_ZVAL(args);
    array_init(args);
  }
  else {
    zval_add_ref(&args);
  }

  // turn the first argument into MongoCode
  if (Z_TYPE_P(code) != IS_OBJECT || 
      Z_OBJCE_P(code) != mongo_ce_Code) {
    zval *obj;
    MAKE_STD_ZVAL(obj);
    object_init_ex(obj, mongo_ce_Code);

    zend_ptr_stack_n_push(&EG(argument_stack), 3, code, 1, NULL);
    zim_MongoCode___construct(1, return_value, return_value_ptr, obj, return_value_used TSRMLS_CC);

    void *holder;
    zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

    code = obj;
  }

  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);

  // create { $eval : code, args : [] }
  zval *zdata;
  MAKE_STD_ZVAL(zdata);
  array_init(zdata);
  add_assoc_zval(zdata, "$eval", code);
  add_assoc_zval(zdata, "args", args);

  zval *zname = zend_read_property(mongo_ce_Mongo, getThis(), "name", strlen("name"), NOISY TSRMLS_CC);
  zend_ptr_stack_n_push(&EG(argument_stack), 5, zlink, zdata, zname, 3, NULL);

  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 5, &holder, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&zdata);
  zval_ptr_dtor(&args);
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

/* {{{ mongo_init_MongoDB_new
 */
zend_object_value mongo_init_MongoDB_new(zend_class_entry *class_type TSRMLS_DC) {
  zval tmp, obj;
  zend_object *object;

  Z_OBJVAL(obj) = zend_objects_new(&object, class_type TSRMLS_CC);
  Z_OBJ_HT(obj) = zend_get_std_object_handlers();
 
  ALLOC_HASHTABLE(object->properties);
  zend_hash_init(object->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
  zend_hash_copy(object->properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

  return Z_OBJVAL(obj);
}
/* }}} */


void mongo_init_MongoDB(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoDB", MongoDB_methods);
  ce.create_object = mongo_init_MongoDB_new;
  mongo_ce_DB = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_OFF", strlen("PROFILING_OFF"), 0 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_SLOW", strlen("PROFILING_SLOW"), 1 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_ON", strlen("PROFILING_ON"), 2 TSRMLS_CC);

  zend_declare_property_null(mongo_ce_DB, "connection", strlen("connection"), ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_ce_DB, "name", strlen("name"), ZEND_ACC_PUBLIC TSRMLS_CC);
}
