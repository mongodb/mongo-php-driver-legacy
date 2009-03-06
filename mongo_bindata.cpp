// mongo_bindata.cpp
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

#include "mongo.h"

extern zend_class_entry *mongo_bindata_class;

/* {{{ proto MongoBinData mongo_bindata___construct(string) 
   Creates a new MongoBinData */
PHP_FUNCTION( mongo_bindata___construct ) {
  char *bin;
  int bin_len, type;

  int argc = ZEND_NUM_ARGS();
  if (argc == 1 &&
      zend_parse_parameters(argc TSRMLS_CC, "s", &bin, &bin_len) != FAILURE) {
    type = mongo::ByteArray;
  }
  else if (argc != 2 ||
           zend_parse_parameters(argc TSRMLS_CC, "sl", &bin, &bin_len, &type) == FAILURE) {
    zend_error( E_ERROR, "incorrect parameter types, expected __construct(string, long)" );
  }
  
  add_property_stringl( getThis(), "bin", bin, strlen(bin), 1 );
  add_property_long( getThis(), "length", bin_len);
  add_property_long( getThis(), "type", type);
}
/* }}} */


/* {{{ proto string mongo_bindata___toString() 
   Returns a string represenation of this MongoBinData */
PHP_FUNCTION( mongo_bindata___toString ) {
  RETURN_STRING( "<Mongo Binary Data>", 1 );
}
/* }}} */


zval* bin_to_php_bin( char *bin, int len, int type ) {
  TSRMLS_FETCH();
  zval *zbin;
    
  MAKE_STD_ZVAL(zbin);
  object_init_ex(zbin, mongo_bindata_class);

  add_property_stringl( zbin, "bin", bin, len, 1 );
  add_property_long( zbin, "length", len);
  add_property_long( zbin, "type", type);
  return zbin;
}
