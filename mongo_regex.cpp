//mongo_regex.cpp
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

extern zend_class_entry *mongo_regex_class;

/* {{{ proto MongoRegex mongo_regex___construct(string) 
   Creates a new MongoRegex */
PHP_FUNCTION( mongo_regex___construct ) {
  char *re, *opts;
  int re_len, opts_len;

  switch (ZEND_NUM_ARGS()) {
  case 0:
    add_property_stringl( getThis(), "regex", "", 0, 1 );
    add_property_stringl( getThis(), "flags", "", 0, 1 );
    break;
  case 1:
    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &re, &re_len) == FAILURE ) {
      zend_error( E_WARNING, "incorrect parameter types" );
      RETURN_FALSE;
    }
    string r = string(re);
    int splitter = r.find_last_of( "/" );

    string newre = r.substr(1, splitter - 1);
    string newopts = r.substr(splitter + 1);

    add_property_stringl( getThis(), "regex", (char*)newre.c_str(), newre.length(), 1 );
    add_property_stringl( getThis(), "flags", (char*)newopts.c_str(), newre.length(), 1 );
    break;
  }
}
/* }}} */


/* {{{ proto string mongo_regex___toString() 
   Returns a string represenation of this MongoRegex in hexidecimal */
PHP_FUNCTION( mongo_regex___toString ) {
  zval *zre = zend_read_property( mongo_regex_class, getThis(), "regex", 5, 0 TSRMLS_CC );
  char *re = Z_STRVAL_P( zre );
  zval *zopts = zend_read_property( mongo_regex_class, getThis(), "flags", 5, 0 TSRMLS_CC );
  char *opts = Z_STRVAL_P( zopts );

  int re_len = strlen(re);
  int opts_len = strlen(opts);
  char field_name[re_len+opts_len+3];

  sprintf( field_name, "/%s/%s", re, opts );
  RETURN_STRING( field_name, 1 );
}
/* }}} */


zval* re_to_mongo_re( char *re, char *flags ) {
  TSRMLS_FETCH();
  zval *zre;
    
  MAKE_STD_ZVAL(zre);
  object_init_ex(zre, mongo_regex_class);

  add_property_stringl( zre, "regex", re, strlen(re), 1 );
  add_property_stringl( zre, "flags", flags, strlen(flags), 1 );
  return zre;
}
