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

zend_class_entry *mongo_ce_MongoDB = NULL;

zend_object_value mongo_init_MongoDB_new(zend_class_entry *class_type) {
  zend_object_value retval;


}


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
  zend_update_property(mongo_ce_MongoDB, getThis(), "connection", strlen("connection"), zconn TSRMLS_CC);
  zend_update_property_string(mongo_ce_MongoDB, getThis(), "name", strlen("name"), name TSRMLS_CC);
}

PHP_METHOD(MongoDB, __toString) {
  zval *name = zend_read_property(mongo_ce_MongoDB, getThis(), "name", strlen("name"), 0 TSRMLS_CC);
  RETURN_STRING(name, 0, 0);
}

PHP_METHOD(MongoDB, selectCollection) {
  //TODO!
}

PHP_METHOD(MongoDB, getGridFS) {
  //TODO
}

PHP_METHOD(MongoDB, getProfilingLevel) {

}

PHP_METHOD(MongoDB, setProfilingLevel) {}
PHP_METHOD(MongoDB, drop) {}
PHP_METHOD(MongoDB, repair) {}
PHP_METHOD(MongoDB, createCollection) {}
PHP_METHOD(MongoDB, dropCollection) {}
PHP_METHOD(MongoDB, listCollections) {}
PHP_METHOD(MongoDB, getCursorInfo) {}
PHP_METHOD(MongoDB, createDBRef) {}
PHP_METHOD(MongoDB, getDBRef) {}
PHP_METHOD(MongoDB, execute) {}

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
  mongo_ce_MongoDB = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_long(mongo_util_ce, "PROFILING_OFF", strlen("PROFILING_OFF"), 0 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_util_ce, "PROFILING_SLOW", strlen("PROFILING_SLOW"), 1 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_util_ce, "PROFILING_ON", strlen("PROFILING_ON"), 2 TSRMLS_CC);

  zend_declare_property_null(mongo_cursor_ce, "connection", strlen("connection"), ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_cursor_ce, "name", strlen("cursor"), ZEND_ACC_PUBLIC TSRMLS_CC);
}
