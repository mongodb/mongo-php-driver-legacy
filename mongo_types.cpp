//mongo_types.cpp
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
#include <sys/time.h>

#include "mongo.h"
#include "bson.h"

extern zend_class_entry *mongo_bindata_class;
extern zend_class_entry *mongo_date_class;
extern zend_class_entry *mongo_id_class;
extern zend_class_entry *mongo_regex_class;

/* {{{ proto MongoId mongo_id___construct() 
   Creates a new MongoId */
PHP_FUNCTION( mongo_id___construct ) {
  char *id;
  int id_len;

  mongo::OID *oid = new mongo::OID();
  if (ZEND_NUM_ARGS() == 1) { 
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &id, &id_len) == FAILURE ) {
      zend_error( E_WARNING, "incorrect parameter types" );
      RETURN_FALSE;
    } else {
      std::string s (id, id_len);
      oid->init(s);
      add_property_stringl( getThis(), "id", id, id_len, 1 );
    }
  }
  else {
    oid->init();
    std::string str = oid->str();
    char *c = (char*)str.c_str();
    add_property_stringl( getThis(), "id", c, strlen( c ), 1 );
  }
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

zval* oid_to_mongo_id( const mongo::OID oid ) {
  TSRMLS_FETCH();
  zval *zoid;
  
  const unsigned char *data = oid.getData();
  char *buf = new char[25];
  char *n = buf;
  int i;
  for(i = 0; i < 12; i++) {
    sprintf(n, "%02x", *data++);
    n += 2;
  } 
  *(n) = '\0';

  MAKE_STD_ZVAL(zoid);
  object_init_ex(zoid, mongo_id_class);
  add_property_stringl( zoid, "id", buf, strlen( buf ), 1 );
  return zoid;
}



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

