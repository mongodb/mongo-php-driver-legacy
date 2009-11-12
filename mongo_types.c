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

#include <stdlib.h>

#ifdef WIN32
#include <time.h>
#else
#include <string.h>
#include <sys/time.h>
#endif

#include <php.h>
#include <zend_exceptions.h>
#if ZEND_MODULE_API_NO >= 20060613
#include <ext/spl/spl_exceptions.h>
#endif
#include <ext/standard/php_rand.h>

#include "mongo_types.h"
#include "php_mongo.h"
#include "db.h"
#include "collection.h"
#include "bson.h"

extern zend_class_entry *mongo_ce_DB,
  *mongo_ce_Exception;

extern zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_Date = NULL,
  *mongo_ce_BinData = NULL,
  *mongo_ce_DBRef = NULL,
  *mongo_ce_Id = NULL,
  *mongo_ce_Code = NULL,
  *mongo_ce_Regex = NULL,
  *mongo_ce_Timestamp = NULL;

void generate_id(char *data TSRMLS_DC) {
  int inc;
  int pid = (int)getpid();
  unsigned t = (unsigned) time(0);
  char *T = (char*)&t;

  data[0] = T[3];
  data[1] = T[2];
  data[2] = T[1];
  data[3] = T[0];

  // 3 bytes machine
  memcpy(data+4, &MonGlo(machine), 3);

  // 2 bytes pid
  memcpy(data+7, &pid, 2);

  // 3 bytes inc
#ifdef WIN32
  inc = php_rand(TSRMLS_C);
#else
  inc = MonGlo(inc);
  MonGlo(inc)++;
#endif

  memcpy(data+9, &inc, 3);
}

int mongo_mongo_id_serialize(zval *struc, unsigned char **serialized_data, zend_uint *serialized_length, zend_serialize_data *var_hash TSRMLS_DC) {
  zval str;
  MONGO_METHOD(MongoId, __toString, &str, struc, 0, NULL);
  *(serialized_length) = Z_STRLEN(str);
  *(serialized_data) = (unsigned char*)Z_STRVAL(str);
  return SUCCESS;
}

int mongo_mongo_id_unserialize(zval **rval, zend_class_entry *ce, const unsigned char* p, zend_uint datalen, zend_unserialize_data* var_hash TSRMLS_DC) {
  zval temp;
  zval str;

  Z_TYPE(str) = IS_STRING;
  Z_STRLEN(str) = 24;
  Z_STRVAL(str) = estrndup((char*)p, 24);

  object_init_ex(*rval, mongo_ce_Id);

  MONGO_METHOD(MongoId, __construct, &temp, *rval, 1, &str);
  efree(Z_STRVAL(str));

  return SUCCESS;
}

static void mongo_mongo_id_free(void *object TSRMLS_DC) {
  mongo_id *id = (mongo_id*)object;
  if (id) {
    if (id->id) {
      efree(id->id);
    }
    zend_object_std_dtor(&id->std TSRMLS_CC);
    efree(id);
  }
}

static zend_object_value mongo_mongo_id_new(zend_class_entry *class_type TSRMLS_DC) {
  zend_object_value retval;
  mongo_id *intern;
  zval *tmp;

  intern = (mongo_id*)emalloc(sizeof(mongo_id));
  memset(intern, 0, sizeof(mongo_id));

  zend_object_std_init(&intern->std, class_type TSRMLS_CC);
  zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, 
                 (void *) &tmp, sizeof(zval *));

  retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, mongo_mongo_id_free, NULL TSRMLS_CC);
  retval.handlers = &mongo_default_handlers;

  return retval;
}

static function_entry MongoId_methods[] = {
  PHP_ME(MongoId, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoId, __toString, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoId(TSRMLS_D) {
  zend_class_entry id; 
  INIT_CLASS_ENTRY(id, "MongoId", MongoId_methods); 

  id.create_object = mongo_mongo_id_new;
  id.serialize = mongo_mongo_id_serialize;
  id.unserialize = mongo_mongo_id_unserialize;

  mongo_ce_Id = zend_register_internal_class(&id TSRMLS_CC); 
}

/* {{{ MongoId::__construct()
 */
PHP_METHOD(MongoId, __construct) {
  zval *id = 0;
  mongo_id *this_id = (mongo_id*)zend_object_store_get_object(getThis() TSRMLS_CC);
  this_id->id = (char*)emalloc(OID_SIZE+1);
  this_id->id[OID_SIZE] = '\0';

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &id) == FAILURE) {
    return;
  }
  
  if (id && 
      Z_TYPE_P(id) == IS_STRING && 
      Z_STRLEN_P(id) == 24) {
    int i;
    for(i=0;i<12;i++) {
      char digit1 = Z_STRVAL_P(id)[i*2], digit2 = Z_STRVAL_P(id)[i*2+1];
      digit1 = digit1 >= 'a' && digit1 <= 'f' ? digit1 -= 87 : digit1;
      digit1 = digit1 >= 'A' && digit1 <= 'F' ? digit1 -= 55 : digit1;
      digit1 = digit1 >= '0' && digit1 <= '9' ? digit1 -= 48 : digit1;
      
      digit2 = digit2 >= 'a' && digit2 <= 'f' ? digit2 -= 87 : digit2;
      digit2 = digit2 >= 'A' && digit2 <= 'F' ? digit2 -= 55 : digit2;
      digit2 = digit2 >= '0' && digit2 <= '9' ? digit2 -= 48 : digit2;
      
      this_id->id[i] = digit1*16+digit2;
    }
  }
  else if (id && 
           Z_TYPE_P(id) == IS_OBJECT &&
           Z_OBJCE_P(id) == mongo_ce_Id) {
    mongo_id *that_id = (mongo_id*)zend_object_store_get_object(id TSRMLS_CC);
    memcpy(this_id->id, that_id->id, OID_SIZE);
  }
  else {
    generate_id(this_id->id TSRMLS_CC);
  }
}
/* }}} */


/* {{{ MongoId::__toString() 
 */
PHP_METHOD(MongoId, __toString) {
  int i;
  mongo_id *this_id;
  char *id_str, *movable;
  char *id;

  this_id = (mongo_id*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED_STRING(this_id->id, MongoId);

  id = (char*)emalloc(25);
  id_str = this_id->id;

  movable = id;
  for(i=0; i<12; i++) {
    int x = *id_str;
    if (*id_str < 0) {
      x = 256 + *id_str;
    }
    sprintf(movable, "%02x", x);
    movable += 2;
    id_str++;
  }

  id[24] = '\0';

  RETURN_STRING(id, NO_DUP);
}
/* }}} */


/* {{{ MongoDate::__construct
 */
PHP_METHOD(MongoDate, __construct) {
  int arg = 0, usec = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll", &arg, &usec) == FAILURE) {
    return;
  }

  if (!arg) {
#ifdef WIN32
    time_t sec = time(0);
    add_property_long(getThis(), "sec", sec);
    add_property_long(getThis(), "usec", 0);
#else
    struct timeval time;
    gettimeofday(&time, NULL);

    add_property_long(getThis(), "sec", time.tv_sec);
    add_property_long(getThis(), "usec", time.tv_usec);
#endif
  }
  else {
    add_property_long(getThis(), "sec", arg);
    add_property_long(getThis(), "usec", usec);
  }
}
/* }}} */


/* {{{ MongoDate::__toString()
 */
PHP_METHOD(MongoDate, __toString) {
  zval *zsec = zend_read_property( mongo_ce_Date, getThis(), "sec", 3, 0 TSRMLS_CC );
  long sec = Z_LVAL_P( zsec );
  zval *zusec = zend_read_property( mongo_ce_Date, getThis(), "usec", 4, 0 TSRMLS_CC );
  long usec = Z_LVAL_P( zusec );
  double dusec = (double)usec/1000000;

  return_value->type = IS_STRING;
  return_value->value.str.len = spprintf(&(return_value->value.str.val), 0, "%.8f %ld", dusec, sec);
}
/* }}} */



static function_entry MongoDate_methods[] = {
  PHP_ME(MongoDate, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDate, __toString, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoDate(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoDate", MongoDate_methods);
  mongo_ce_Date = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_long(mongo_ce_Date, "sec", strlen("sec"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_Date, "usec", strlen("usec"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);
}

/* {{{ MongoBinData::__construct() 
 */
PHP_METHOD(MongoBinData, __construct) {
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
  
  add_property_stringl( getThis(), "bin", bin, bin_len, 1 );
  add_property_long( getThis(), "type", type);
}
/* }}} */


/* {{{ MongoBinData::__toString() 
 */
PHP_METHOD(MongoBinData, __toString) {
  RETURN_STRING( "<Mongo Binary Data>", 1 );
}
/* }}} */


static function_entry MongoBinData_methods[] = {
  PHP_ME(MongoBinData, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoBinData, __toString, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoBinData(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoBinData", MongoBinData_methods);
  mongo_ce_BinData = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_string(mongo_ce_BinData, "bin", strlen("bin"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_BinData, "type", strlen("type"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);
}

/* {{{ MongoRegex::__construct() 
 */
PHP_METHOD(MongoRegex, __construct) {
  zval *regex;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &regex) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(regex) == IS_OBJECT &&
      Z_OBJCE_P(regex) == mongo_ce_Regex) {
    zval *oregex, *oflags;

    oregex = zend_read_property(mongo_ce_Regex, regex, "regex", strlen("regex"), NOISY TSRMLS_CC);
    zval_add_ref(&oregex);
    zend_update_property(mongo_ce_Regex, getThis(), "regex", strlen("regex"), oregex TSRMLS_CC);

    oflags = zend_read_property(mongo_ce_Regex, regex, "flags", strlen("flags"), NOISY TSRMLS_CC);
    zval_add_ref(&oflags);
    zend_update_property(mongo_ce_Regex, getThis(), "flags", strlen("flags"), oflags TSRMLS_CC);
  }
  else if (Z_TYPE_P(regex) == IS_STRING) {
    int pattern_len, flags_len;
    char *re = Z_STRVAL_P(regex);
    char *eopattern = strrchr(re, '/');

    if (!eopattern) {
      zend_throw_exception(mongo_ce_Exception, "invalid regex", 0 TSRMLS_CC);
      return;
    }

    pattern_len = eopattern - re - 1;

    if (pattern_len < 0) {
      zend_throw_exception(mongo_ce_Exception, "invalid regex", 0 TSRMLS_CC);
      return;
    }

    // move beyond the second '/' in /foo/bar
    eopattern++;
    flags_len = Z_STRLEN_P(regex) - (eopattern-re);

    add_property_stringl( getThis(), "regex", re+1, pattern_len, 1);
    add_property_stringl( getThis(), "flags", eopattern, flags_len, 1);
  }
  else {
#   if ZEND_MODULE_API_NO >= 20060613
    zend_throw_exception(spl_ce_InvalidArgumentException, "MongoRegex::__construct(): argument must be a string or MongoRegex", 0 TSRMLS_CC);
#   else
    zend_throw_exception(zend_exception_get_default(), "MongoRegex::__construct(): argument must be a string or MongoRegex", 0 TSRMLS_CC);
#   endif
    return;
  }
}
/* }}} */


/* {{{ MongoRegex::__toString()
 */
PHP_METHOD(MongoRegex, __toString) {
  char *field_name;
  zval *zre = zend_read_property( mongo_ce_Regex, getThis(), "regex", 5, 0 TSRMLS_CC );
  char *re = Z_STRVAL_P( zre );
  zval *zopts = zend_read_property( mongo_ce_Regex, getThis(), "flags", 5, 0 TSRMLS_CC );
  char *opts = Z_STRVAL_P( zopts );

  spprintf(&field_name, 0, "/%s/%s", re, opts );
  RETVAL_STRING(field_name, 1);
  efree(field_name);
}
/* }}} */


static function_entry MongoRegex_methods[] = {
  PHP_ME(MongoRegex, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoRegex, __toString, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoRegex(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoRegex", MongoRegex_methods);
  mongo_ce_Regex = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_string(mongo_ce_Regex, "regex", strlen("regex"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_string(mongo_ce_Regex, "flags", strlen("flags"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
}



/* {{{ MongoCode::__construct(string) 
 */
PHP_METHOD(MongoCode, __construct) {
  zval *code = 0, *zcope = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &code, &zcope) == FAILURE) {
    return;
  }
  convert_to_string(code);

  zend_update_property(mongo_ce_Code, getThis(), "code", strlen("code"), code TSRMLS_CC);

  if (!zcope) {
    ALLOC_INIT_ZVAL(zcope);
    array_init(zcope);
  }
  else {
    zval_add_ref(&zcope);
  }

  zend_update_property(mongo_ce_Code, getThis(), "scope", strlen("scope"), zcope TSRMLS_CC);

  // get rid of extra ref
  zval_ptr_dtor(&zcope);
}
/* }}} */


/* {{{ MongoCode::__toString() 
 */
PHP_METHOD(MongoCode, __toString ) {
  zval *zode = zend_read_property(mongo_ce_Code, getThis(), "code", 4, 0 TSRMLS_CC);
  RETURN_STRING(Z_STRVAL_P(zode), 1 );
}
/* }}} */


static function_entry MongoCode_methods[] = {
  PHP_ME(MongoCode, __construct, NULL, ZEND_ACC_PUBLIC )
  PHP_ME(MongoCode, __toString, NULL, ZEND_ACC_PUBLIC )
  { NULL, NULL, NULL }
};

void mongo_init_MongoCode(TSRMLS_D) {
  zend_class_entry ce; 
  INIT_CLASS_ENTRY(ce, "MongoCode", MongoCode_methods); 
  mongo_ce_Code = zend_register_internal_class(&ce TSRMLS_CC); 

  zend_declare_property_string(mongo_ce_Code, "code", strlen("code"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Code, "scope", strlen("scope"), ZEND_ACC_PUBLIC TSRMLS_CC);
}

/* {{{ MongoCode::create()
 */
PHP_METHOD(MongoDBRef, create) {
  zval *zns, *zid;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &zns, &zid) == FAILURE) {
    return;
  }
  convert_to_string(zns);

  array_init(return_value);
  add_assoc_zval(return_value, "$ref", zns); 
  zval_add_ref(&zns);
  add_assoc_zval(return_value, "$id", zid); 
  zval_add_ref(&zid);
}
/* }}} */

/* {{{ MongoDBRef::isRef()
 */
PHP_METHOD(MongoDBRef, isRef) {
  zval *ref;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE ||
      IS_SCALAR_P(ref)) {
    return;
  }

  if (zend_hash_exists(HASH_P(ref), "$ref", 5) &&
      zend_hash_exists(HASH_P(ref), "$id", 4)) {
    RETURN_TRUE;
  }
  RETURN_FALSE;
}
/* }}} */

/* {{{ MongoDBRef::get()
 */
PHP_METHOD(MongoDBRef, get) {
  zval *db, *ref, *collection, *query;
  zval **ns, **id;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &db, mongo_ce_DB, &ref) == FAILURE) {
    return;
  }
  
  if (zend_hash_find(HASH_P(ref), "$ref", 5, (void**)&ns) == FAILURE ||
      zend_hash_find(HASH_P(ref), "$id", 4, (void**)&id) == FAILURE) {
    RETURN_NULL();
  }
  
  MAKE_STD_ZVAL(collection);
  MONGO_METHOD(MongoDB, selectCollection, collection, db, 1, *ns);
  
  MAKE_STD_ZVAL(query);
  array_init(query);
  add_assoc_zval(query, "_id", *id);
  zval_add_ref(id);
  
  MONGO_METHOD(MongoCollection, findOne, return_value, collection, 1, query);
  zval_ptr_dtor(&collection);
  zval_ptr_dtor(&query);
}
/* }}} */

static function_entry MongoDBRef_methods[] = {
  PHP_ME(MongoDBRef, create, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoDBRef, isRef, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoDBRef, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  { NULL, NULL, NULL }
};


void mongo_init_MongoDBRef(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoDBRef", MongoDBRef_methods);
  mongo_ce_DBRef = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_string(mongo_ce_DBRef, "refKey", strlen("refKey"), "$ref", ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
  zend_declare_property_string(mongo_ce_DBRef, "idKey", strlen("idKey"), "$id", ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
}

static function_entry MongoTimestamp_methods[] = {
  PHP_ME(MongoTimestamp, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoTimestamp, __toString, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoTimestamp(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoTimestamp", MongoTimestamp_methods);
  mongo_ce_Timestamp = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_long(mongo_ce_Timestamp, "sec", strlen("sec"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_Timestamp, "inc", strlen("inc"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);
}

/*
 * Timestamp is 4 bytes of seconds since epoch and 4 bytes of increment.
 */
PHP_METHOD(MongoTimestamp, __construct) {
  zval *sec = 0, *inc = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &sec, &inc) == FAILURE) {
    return;
  }

  if (sec) {
    /* this isn't the most elegant thing ever, but if someone passes in
     * something that isn't a long, convert it to a long (same with inc).
     */
    convert_to_long(sec);
    zend_update_property(mongo_ce_Timestamp, getThis(), "sec", strlen("sec"), sec TSRMLS_CC);
  }
  else {
    zend_update_property_long(mongo_ce_Timestamp, getThis(), "sec", strlen("sec"), time(0) TSRMLS_CC);
  }

  if (inc) {
    convert_to_long(inc);
    zend_update_property(mongo_ce_Timestamp, getThis(), "inc", strlen("inc"), inc TSRMLS_CC);
  }
  else {
    zend_update_property_long(mongo_ce_Timestamp, getThis(), "inc", strlen("inc"), MonGlo(ts_inc)++ TSRMLS_CC);
  }
}

/*
 * Just convert the seconds field to a string.
 */
PHP_METHOD(MongoTimestamp, __toString) {
  char *str;
  zval *sec = zend_read_property(mongo_ce_Timestamp, getThis(), "sec", strlen("sec"), NOISY TSRMLS_CC);
  spprintf(&str, 0, "%ld", Z_LVAL_P(sec));
  RETURN_STRING(str, 0);
}
