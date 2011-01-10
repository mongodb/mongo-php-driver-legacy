// connect.c
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

#include "connect.h"
#include "php_mongo.h"

extern zend_class_entry *mongo_ce_Mongo;
ZEND_EXTERN_MODULE_GLOBALS(mongo);

void mongo_util_conn_get_buildinfo(zval *this_ptr TSRMLS_DC) {
  zval *result, *data, *admin, *db, **max = 0;

  // "admin"
  MAKE_STD_ZVAL(admin);
  ZVAL_STRING(admin, "admin", 1);
  
  MAKE_STD_ZVAL(db);
  ZVAL_NULL(db);
  MONGO_METHOD1(Mongo, selectDB, db, getThis(), admin);
  
  zval_ptr_dtor(&admin);
      
  MAKE_STD_ZVAL(result);
  ZVAL_NULL(result);
  
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "buildinfo", 1);
  
  MONGO_CMD(result, db); 

  zval_ptr_dtor(&data);
  zval_ptr_dtor(&db);

  if (zend_hash_find(HASH_P(result), "maxBsonObjectSize",
                     sizeof("maxBsonObjectSize"), (void**)&max) == SUCCESS){
    MonGlo(max_doc_size) = Z_LVAL_PP(max);
  }
  
  zval_ptr_dtor(&result); 
}

