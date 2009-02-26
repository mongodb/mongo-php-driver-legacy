//mongo_date.cpp
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
#include <sys/time.h>

#include "mongo.h"

extern zend_class_entry *mongo_date_class;

/* {{{ proto MongoDate mongo_date___construct() 
   Creates a new MongoDate */
PHP_FUNCTION( mongo_date___construct ) {
  timeval time;
  long ltime;

  if (ZEND_NUM_ARGS() == 0) {
    gettimeofday(&time, NULL);
    add_property_long( getThis(), "sec", time.tv_sec );
    add_property_long( getThis(), "usec", time.tv_usec );
  }
  else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ltime) == SUCCESS) {
    add_property_long( getThis(), "sec", ltime );
    add_property_long( getThis(), "usec", 0 );
  }
  else {
    zend_error( E_WARNING, "incorrect parameter types, expected: __construct( long )" );
    RETURN_FALSE;
  }
}
/* }}} */


/* {{{ proto string mongo_date___toString() 
   Returns a string represenation of this MongoDate */
PHP_FUNCTION( mongo_date___toString ) {
  zval *zsec = zend_read_property( mongo_date_class, getThis(), "sec", 3, 0 TSRMLS_CC );
  long sec = Z_LVAL_P( zsec );
  zval *zusec = zend_read_property( mongo_date_class, getThis(), "usec", 4, 0 TSRMLS_CC );
  long usec = Z_LVAL_P( zusec );
  double dusec = (double)usec/1000000;

  char time_str[21];

  sprintf( time_str, "%.8f %ld", dusec, sec );
  RETURN_STRING( time_str, 1 );
}
/* }}} */


zval* date_to_mongo_date( unsigned long long date ) {
  TSRMLS_FETCH();
  zval *zd;
    
  MAKE_STD_ZVAL(zd);
  object_init_ex(zd, mongo_date_class);

  long usec = (date % 1000) * 1000;
  long sec = date / 1000;

  add_property_long( zd, "sec", sec );
  add_property_long( zd, "usec", usec );
  return zd;
}
