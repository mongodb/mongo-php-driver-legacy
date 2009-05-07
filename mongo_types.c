//mongo_types.c
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

#ifdef WIN32
#include <time.h>
#else
#include <string.h>
#include <sys/time.h>
#endif

#include <php.h>

#include "mongo_types.h"
#include "mongo.h"
#include "bson.h"

extern zend_class_entry *mongo_bindata_class;
extern zend_class_entry *mongo_code_class;
extern zend_class_entry *mongo_date_class;
extern zend_class_entry *mongo_id_class;
extern zend_class_entry *mongo_regex_class;

zend_class_entry *mongo_dbref_ce = NULL;

// takes an allocated but not initialized zval
// turns it into an MongoId
void create_id(zval *zoid, char *data TSRMLS_DC) {
  object_init_ex(zoid, mongo_id_class);

  if (data) {
    add_property_stringl(zoid, "id", data, OID_SIZE, DUP);
  }
  else {
    char id[12];
    generate_id(id);
    add_property_stringl(zoid, "id", id, OID_SIZE, DUP);
  }
}

void generate_id(char *data) {
  // THIS WILL ONLY WORK ON *NIX
  FILE *rand = fopen("/dev/urandom", "rb");
  char machine[4], inc[4];
  fgets(machine, 4, rand);
  fgets(inc, 4, rand);
    
  unsigned t = (unsigned) time(0);
  char *T = (char*)&t;
  data[0] = T[3];
  data[1] = T[2];
  data[2] = T[1];
  data[3] = T[0];

  memcpy(data+4, machine, 4);
  data[8] = inc[3];
  data[9] = inc[2];
  data[10] = inc[1];
  data[11] = inc[0];
  
  fclose(rand);
}

static function_entry MongoId_methods[] = {
  PHP_ME(MongoId, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoId, __toString, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoId(TSRMLS_D) {
  zend_class_entry id; 
  INIT_CLASS_ENTRY(id, "MongoId", MongoId_methods); 
  mongo_id_class = zend_register_internal_class_ex(&id, mongo_id_class, NULL TSRMLS_CC); 
}

/* {{{ MongoId::__construct()
 */
PHP_METHOD(MongoId, __construct) {
  char *id;
  int id_len;
  char data[12];

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &id, &id_len) == SUCCESS &&
      id_len == 24) {
    int i;
    for(i=0;i<12;i++) {
      char digit1 = id[i*2], digit2 = id[i*2+1];
      digit1 = digit1 >= 'a' && digit1 <= 'f' ? digit1 -= 87 : digit1;
      digit1 = digit1 >= 'A' && digit1 <= 'F' ? digit1 -= 55 : digit1;
      digit1 = digit1 >= '0' && digit1 <= '9' ? digit1 -= 48 : digit1;
      
      digit2 = digit2 >= 'a' && digit2 <= 'f' ? digit2 -= 87 : digit2;
      digit2 = digit2 >= 'A' && digit2 <= 'F' ? digit2 -= 55 : digit2;
      digit2 = digit2 >= '0' && digit2 <= '9' ? digit2 -= 48 : digit2;
      
      data[i] = digit1*16+digit2;
    }
  }
  else {
    generate_id(data);
  }
    
  add_property_stringl(getThis(), "id", data, OID_SIZE, DUP);
}
/* }}} */


/* {{{ MongoId::__toString() 
 */
PHP_METHOD(MongoId, __toString) {
  int i;
  zval *zid = zend_read_property(mongo_id_class, getThis(), "id", 2, 0 TSRMLS_CC);
  char *foo = zid->value.str.val;

  char id[24];
  char *n = id;
  for(i=0; i<12; i++) {
    int x = *foo;
    if (*foo < 0) {
      x = 256 + *foo;
    }
    sprintf(n, "%02x", x);
    n += 2;
    foo++;
  }

  RETURN_STRING(id, DUP);
}
/* }}} */


/* {{{ mongo_date___construct() 
 */
PHP_FUNCTION( mongo_date___construct ) {
  struct timeval time;
  long sec, usec;

  int argc = ZEND_NUM_ARGS();
  switch(argc) {
  case 0: {
#ifdef WIN32
    SYSTEMTIME systime;
	GetSystemTime(&systime);
	time.tv_sec = systime.wSecond;
	time.tv_usec = systime.wMilliseconds * 1000;
#else
    gettimeofday(&time, NULL);
#endif
    add_property_long( getThis(), "sec", time.tv_sec );
    add_property_long( getThis(), "usec", time.tv_usec );
    break;
  }
  case 1: {
    if (zend_parse_parameters(argc TSRMLS_CC, "l", &sec) == SUCCESS) {
      long long temp = sec;
      long long t_sec = temp/1000;
      long long t_usec = temp*1000 % 1000000;

      // back to longs
      sec = (long)t_sec;
      usec = (long)t_usec;

      add_property_long( getThis(), "sec", sec);
      add_property_long( getThis(), "usec", usec);
    }
    break;
  }
  case 2: {
    if (zend_parse_parameters(argc TSRMLS_CC, "ll", &sec, &usec) == SUCCESS) {
      add_property_long(getThis(), "sec", sec);
      add_property_long(getThis(), "usec", usec);
    }
    break;
  }
  }
}
/* }}} */


/* {{{ mongo_date___toString() 
 */
PHP_FUNCTION( mongo_date___toString ) {
  zval *zsec = zend_read_property( mongo_date_class, getThis(), "sec", 3, 0 TSRMLS_CC );
  long sec = Z_LVAL_P( zsec );
  zval *zusec = zend_read_property( mongo_date_class, getThis(), "usec", 4, 0 TSRMLS_CC );
  long usec = Z_LVAL_P( zusec );
  double dusec = (double)usec/1000000;

  return_value->type = IS_STRING;
  return_value->value.str.len = spprintf(&(return_value->value.str.val), 0, "%.8f %ld", dusec, sec);
}
/* }}} */


zval* bson_to_zval_date(unsigned long long date TSRMLS_DC) {
  zval *zd;
    
  MAKE_STD_ZVAL(zd);
  object_init_ex(zd, mongo_date_class);

  long usec = (date % 1000) * 1000;
  long sec = date / 1000;

  add_property_long( zd, "sec", sec );
  add_property_long( zd, "usec", usec );
  return zd;
}

unsigned long long zval_to_bson_date(zval **data TSRMLS_DC) {
  zval *zsec = zend_read_property( mongo_date_class, *data, "sec", 3, 0 TSRMLS_CC );
  long sec = Z_LVAL_P( zsec );
  zval *zusec = zend_read_property( mongo_date_class, *data, "usec", 4, 0 TSRMLS_CC );
  long usec = Z_LVAL_P( zusec );
  return (unsigned long long)(sec * 1000) + (unsigned long long)(usec/1000);
}


/* {{{ mongo_bindata___construct(string) 
 */
PHP_FUNCTION(mongo_bindata___construct) {
  char *bin;
  int bin_len, type;

  int argc = ZEND_NUM_ARGS();
  if (argc == 1 &&
      zend_parse_parameters(argc TSRMLS_CC, "s", &bin, &bin_len) != FAILURE) {
    type = BIN_BYTE_ARRAY;
  }
  else if (argc != 2 ||
           zend_parse_parameters(argc TSRMLS_CC, "sl", &bin, &bin_len, &type) == FAILURE) {
    zend_error( E_ERROR, "incorrect parameter types, expected __construct(string, long)" );
  }
  
  add_property_long( getThis(), "length", bin_len);
  add_property_stringl( getThis(), "bin", bin, bin_len, 1 );
  add_property_long( getThis(), "type", type);
}
/* }}} */


/* {{{ mongo_bindata___toString() 
 */
PHP_FUNCTION(mongo_bindata___toString) {
  RETURN_STRING( "<Mongo Binary Data>", 1 );
}
/* }}} */


zval* bson_to_zval_bin( char *bin, int len, int type TSRMLS_DC) {
  zval *zbin;
    
  MAKE_STD_ZVAL(zbin);
  object_init_ex(zbin, mongo_bindata_class);

  add_property_stringl( zbin, "bin", bin, len, 1 );
  add_property_long( zbin, "length", len);
  add_property_long( zbin, "type", type);
  return zbin;
}

int zval_to_bson_bin(zval **data, char **bin, int *len TSRMLS_DC) {
  zval *zbin = zend_read_property( mongo_bindata_class, *data, "bin", 3, 0 TSRMLS_CC );
  *(bin) = Z_STRVAL_P( zbin );
  zval *zlen = zend_read_property( mongo_bindata_class, *data, "length", 6, 0 TSRMLS_CC );
  *(len) = Z_LVAL_P( zlen );
  zval *ztype = zend_read_property( mongo_bindata_class, *data, "type", 4, 0 TSRMLS_CC );
  return (int)Z_LVAL_P( ztype );
}


/* {{{ mongo_regex___construct(string) 
 */
PHP_FUNCTION( mongo_regex___construct ) {
  char *re;
  int re_len;

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

    char *eopattern = strrchr(re, '/');
    int pattern_len = eopattern - re - 1;

    // move beyond the second '/' in /foo/bar 
    eopattern++;
    int flags_len = re_len - (eopattern-re);

    add_property_stringl( getThis(), "regex", re+1, pattern_len, 1);
    add_property_stringl( getThis(), "flags", eopattern, flags_len, 1);
    break;
  }
}
/* }}} */


/* {{{ mongo_regex___toString() 
 */
PHP_FUNCTION( mongo_regex___toString ) {
  zval *zre = zend_read_property( mongo_regex_class, getThis(), "regex", 5, 0 TSRMLS_CC );
  char *re = Z_STRVAL_P( zre );
  zval *zopts = zend_read_property( mongo_regex_class, getThis(), "flags", 5, 0 TSRMLS_CC );
  char *opts = Z_STRVAL_P( zopts );

  int re_len = strlen(re);
  int opts_len = strlen(opts);
  char *field_name;

  spprintf(&field_name, 0, "/%s/%s", re, opts );
  RETVAL_STRING(field_name, 1);
  efree(field_name);
}
/* }}} */


zval* bson_to_zval_regex( char *re, char *flags TSRMLS_DC) {
  zval *zre;
    
  MAKE_STD_ZVAL(zre);
  object_init_ex(zre, mongo_regex_class);

  add_property_stringl( zre, "regex", re, strlen(re), 1 );
  add_property_stringl( zre, "flags", flags, strlen(flags), 1 );
  return zre;
}

void zval_to_bson_regex(zval **data, char **re, char **flags TSRMLS_DC) {
  zval *zre = zend_read_property( mongo_regex_class, *data, "regex", 5, 0 TSRMLS_CC );
  *(re) = Z_STRVAL_P(zre);
  zval *zflags = zend_read_property( mongo_regex_class, *data, "flags", 5, 0 TSRMLS_CC );
  *(flags) = Z_STRVAL_P(zflags);
}


/* {{{ mongo_code___construct(string) 
 */
PHP_FUNCTION( mongo_code___construct ) {
  char *code;
  int code_len;
  zval *zcope;

  int argc = ZEND_NUM_ARGS();
  switch (argc) {
  case 1:
    if (zend_parse_parameters(argc TSRMLS_CC, "s", &code, &code_len) == FAILURE) {
      zend_error( E_ERROR, "incorrect parameter types, expected __construct(string[, array])" );
      RETURN_FALSE;
    }
    ALLOC_INIT_ZVAL(zcope);
    array_init(zcope);
    add_property_zval( getThis(), "scope", zcope );
    // get rid of extra ref
    zval_ptr_dtor(&zcope);
    break;
  case 2:
    if (zend_parse_parameters(argc TSRMLS_CC, "sa", &code, &code_len, &zcope) == FAILURE) {
      zend_error( E_ERROR, "incorrect parameter types, expected __construct(string[, array])" );
      RETURN_FALSE;
    }  
    add_property_zval( getThis(), "scope", zcope );  
    break;
  default:
    zend_error( E_WARNING, "expected 1 or 2 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }

  add_property_stringl( getThis(), "code", code, code_len, 1 );
}
/* }}} */


/* {{{ mongo_code___toString() 
 */
PHP_FUNCTION( mongo_code___toString ) {
  zval *zode = zend_read_property( mongo_code_class, getThis(), "code", 4, 0 TSRMLS_CC );
  char *code = Z_STRVAL_P( zode );
  RETURN_STRING( code, 1 );
}
/* }}} */


PHP_METHOD(MongoDBRef, create) {
  zval **zns, **zid;

  int argc = ZEND_NUM_ARGS();
  if (argc != 2) {
    ZEND_WRONG_NUM_PARAMS();
  }
  zend_get_parameters_ex(argc, &zns, &zid);

  convert_to_string(*zns);

  zval *ref;
  ALLOC_INIT_ZVAL(ref);
  array_init(ref);
  add_assoc_zval(ref, "$ref", *zns); 
  add_assoc_zval(ref, "$id", *zid); 

  RETURN_ZVAL(ref, 0, 1);
}

PHP_METHOD(MongoDBRef, isRef) {
  zval **zobj;

  int argc = ZEND_NUM_ARGS();
  if (argc != 1) {
    ZEND_WRONG_NUM_PARAMS();
  }
  zend_get_parameters_ex(argc, &zobj);

  if (Z_TYPE_PP(zobj) == IS_ARRAY) {
    HashTable *h = Z_ARRVAL_PP(zobj);
    if (zend_hash_find(h, "$ref", 5, NULL) == SUCCESS &&
        zend_hash_find(h, "$id", 4, NULL) == SUCCESS)
      RETURN_TRUE;
  }
  RETURN_FALSE;
}

PHP_METHOD(MongoDBRef, get) {
  zval **zdb, **zref;

  int argc = ZEND_NUM_ARGS();
  if (argc != 2) {
    ZEND_WRONG_NUM_PARAMS();
  }
  zend_get_parameters_ex(argc, &zdb, &zref);

}

static function_entry MongoDBRef_methods[] = {
  PHP_ME(MongoDBRef, create, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoDBRef, isRef, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoDBRef, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
};


void mongo_init_MongoDBRef(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoDBRef", MongoDBRef_methods);
  mongo_dbref_ce = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_string(mongo_dbref_ce, "refKey", strlen("refKey"), "$ref", ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
  zend_declare_property_string(mongo_dbref_ce, "idKey", strlen("idKey"), "$id", ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
}

