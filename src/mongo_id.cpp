//mongo_id.cpp
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
#include <mongo/client/dbclient.h>
#include <string.h>

#include "mongo.h"

extern zend_class_entry *mongo_id_class;

/* {{{ proto MongoId mongo_id___construct() 
   Creates a new MongoId */
PHP_FUNCTION( mongo_id___construct ) {
  mongo::OID *oid = new mongo::OID();
  oid->init();
  std::string str = oid->str();
  char *c = (char*)str.c_str();
  add_property_stringl( getThis(), "id", c, strlen( c ), 1 );
}
/* }}} */


/* {{{ proto string mongo_id___toString() 
   Returns a string represenation of this MongoId in hexidecimal */
PHP_FUNCTION( mongo_id___toString ) {
  zval *zid = zend_read_property( mongo_id_class, getThis(), "id", 2, 0 TSRMLS_CC );
  char *id = Z_STRVAL_P( zid );
  RETURN_STRING( id, 1 );
}
/* }}} */

zval* oid_to_mongo_id( mongo::OID oid ) {
  TSRMLS_FETCH();
  zval *zoid;
  
  std::string str = oid.str();
  char *c = (char*)str.c_str();
  
  MAKE_STD_ZVAL(zoid);
  object_init_ex(zoid, mongo_id_class);
  add_property_stringl( zoid, "id", c, strlen( c ), 1 );
  return zoid;
}
