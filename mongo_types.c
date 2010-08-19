//mongo_types.c
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

#include <stdlib.h>

#ifdef WIN32
#include <time.h>
#else
#include <string.h>
#include <sys/time.h>
#endif

#include <php.h>
#include <zend_exceptions.h>
#include <ext/standard/php_rand.h>

#include "mongo_types.h"
#include "php_mongo.h"
#include "db.h"
#include "collection.h"
#include "bson.h"

extern zend_class_entry *mongo_ce_DB,
  *mongo_ce_Exception;

extern zend_object_handlers mongo_default_handlers,
  mongo_id_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_Date = NULL,
  *mongo_ce_BinData = NULL,
  *mongo_ce_DBRef = NULL,
  *mongo_ce_Id = NULL,
  *mongo_ce_Code = NULL,
  *mongo_ce_Regex = NULL,
  *mongo_ce_Timestamp = NULL,
  *mongo_ce_Int32 = NULL,
  *mongo_ce_Int64 = NULL;

void generate_id(char *data TSRMLS_DC) {
  int inc;

#ifdef WIN32
  int pid = GetCurrentThreadId();
#else
  int pid = (int)getpid();
#endif

  unsigned t = (unsigned) time(0);
  char *T = (char*)&t, 
    *M = (char*)&MonGlo(machine), 
    *P = (char*)&pid, 
    *I = (char*)&inc;

  // inc
  inc = MonGlo(inc);
  MonGlo(inc)++;

  // actually generate the MongoId
#if PHP_C_BIGENDIAN
  // 4 bytes ts
  memcpy(data, T, 4);

  // we add 1 or 2 to the pointers so we don't end up with all 0s, as the
  // interesting stuff is at the end for big endian systems

  // 3 bytes machine
  memcpy(data+4, M+1, 3);

  // 2 bytes pid
  memcpy(data+7, P+2, 2);

  // 3 bytes inc
  memcpy(data+9, I+1, 3);
#else
  // 4 bytes ts
  data[0] = T[3];
  data[1] = T[2];
  data[2] = T[1];
  data[3] = T[0];

  // 3 bytes machine
  memcpy(data+4, M, 3);

  // 2 bytes pid
  memcpy(data+7, P, 2);

  // 3 bytes inc
  memcpy(data+9, I, 3);
#endif /* PHP_C_BIGENDIAN */
}

int php_mongo_id_serialize(zval *struc, unsigned char **serialized_data, zend_uint *serialized_length, zend_serialize_data *var_hash TSRMLS_DC) {
  zval str;
  MONGO_METHOD(MongoId, __toString, &str, struc);
  *(serialized_length) = Z_STRLEN(str);
  *(serialized_data) = (unsigned char*)Z_STRVAL(str);
  return SUCCESS;
}

int php_mongo_id_unserialize(zval **rval, zend_class_entry *ce, const unsigned char* p, zend_uint datalen, zend_unserialize_data* var_hash TSRMLS_DC) {
  zval temp;
  zval str;

  Z_TYPE(str) = IS_STRING;
  Z_STRLEN(str) = 24;
  Z_STRVAL(str) = estrndup((char*)p, 24);

  object_init_ex(*rval, mongo_ce_Id);

  MONGO_METHOD1(MongoId, __construct, &temp, *rval, &str);
  efree(Z_STRVAL(str));

  return SUCCESS;
}

int php_mongo_compare_ids(zval *o1, zval *o2 TSRMLS_DC) {

  if (Z_TYPE_P(o1) == IS_OBJECT && Z_TYPE_P(o2) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(o1), mongo_ce_Id TSRMLS_CC) &&
      instanceof_function(Z_OBJCE_P(o2), mongo_ce_Id TSRMLS_CC)) {
    int i;

    mongo_id *id1 = (mongo_id*)zend_object_store_get_object(o1 TSRMLS_CC);
    mongo_id *id2 = (mongo_id*)zend_object_store_get_object(o2 TSRMLS_CC);

    for (i=0; i<12; i++) {
      if (id1->id[i] < id2->id[i]) {
        return -1;
      }
      else if (id1->id[i] > id2->id[i]) {
        return 1;
      }
    }
    return 0;
  }

  return 1;
}

static void php_mongo_id_free(void *object TSRMLS_DC) {
  mongo_id *id = (mongo_id*)object;
  if (id) {
    if (id->id) {
      efree(id->id);
    }
    zend_object_std_dtor(&id->std TSRMLS_CC);
    efree(id);
  }
}

static zend_object_value php_mongo_id_new(zend_class_entry *class_type TSRMLS_DC) {
  zend_object_value retval;
  mongo_id *intern;
  zval *tmp;

  intern = (mongo_id*)emalloc(sizeof(mongo_id));
  memset(intern, 0, sizeof(mongo_id));

  zend_object_std_init(&intern->std, class_type TSRMLS_CC);
  zend_hash_copy(intern->std.properties,
     &class_type->default_properties,
     (copy_ctor_func_t) zval_add_ref,
     (void *) &tmp,
     sizeof(zval *));

  retval.handle = zend_objects_store_put(intern,
     (zend_objects_store_dtor_t) zend_objects_destroy_object,
     php_mongo_id_free, NULL TSRMLS_CC);
  retval.handlers = &mongo_id_handlers;

  return retval;
}

static function_entry MongoId_methods[] = {
  PHP_ME(MongoId, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoId, __toString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoId, __set_state, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoId, getTimestamp, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoId, getHostname, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  { NULL, NULL, NULL }
};

void mongo_init_MongoId(TSRMLS_D) {
  zend_class_entry id; 
  INIT_CLASS_ENTRY(id, "MongoId", MongoId_methods); 

  id.create_object = php_mongo_id_new;
  id.serialize = php_mongo_id_serialize;
  id.unserialize = php_mongo_id_unserialize;

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
      digit1 = digit1 >= 'a' && digit1 <= 'f' ? digit1 - 87 : digit1;
      digit1 = digit1 >= 'A' && digit1 <= 'F' ? digit1 - 55 : digit1;
      digit1 = digit1 >= '0' && digit1 <= '9' ? digit1 - 48 : digit1;
      
      digit2 = digit2 >= 'a' && digit2 <= 'f' ? digit2 - 87 : digit2;
      digit2 = digit2 >= 'A' && digit2 <= 'F' ? digit2 - 55 : digit2;
      digit2 = digit2 >= '0' && digit2 <= '9' ? digit2 - 48 : digit2;
      
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

/* {{{ MongoId::__set_state() 
 */
PHP_METHOD(MongoId, __set_state) {
  zval temp, *dummy;

  MAKE_STD_ZVAL(dummy);
  ZVAL_STRING(dummy, "000000000000000000000000", 1);

  object_init_ex(return_value, mongo_ce_Id);
  MONGO_METHOD1(MongoId, __construct, &temp, return_value, dummy);

  zval_ptr_dtor(&dummy);
}
/* }}} */

/* {{{ MongoId::getTimestamp
 */
PHP_METHOD(MongoId, getTimestamp) {
  int ts = 0, i;
  mongo_id *id = (mongo_id*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED_STRING(id->id, MongoId);
  
  for (i=0; i<4; i++) {
    int x = ((int)id->id[i] < 0) ? 256+id->id[i] : id->id[i];
    ts = (ts*256) + x;
  }

  RETURN_LONG(ts);
}
/* }}} */

/* {{{ MongoId::getHostname
 */
PHP_METHOD(MongoId, getHostname) {
  char hostname[256];
  gethostname(hostname, 256);
  RETURN_STRING(hostname, 1);
}
/* }}} */

/* {{{ MongoDate::__construct
 */
PHP_METHOD(MongoDate, __construct) {
  zval *arg1 = 0, *arg2 = 0;

  // using "ll" caused segfaults in some 64-bit suhosin-patched php systems
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &arg1, &arg2) == FAILURE) {
    return;
  }

  switch (ZEND_NUM_ARGS()) {
  case 2:
    if (Z_TYPE_P(arg2) != IS_LONG) {
      zend_error(E_WARNING, "MongoDate:__construct() expects integer values");
      return;
    }

    zend_update_property(mongo_ce_Date, getThis(), "usec", strlen("usec"), arg2 TSRMLS_CC);

    // fallthrough
  case 1:
    if (Z_TYPE_P(arg1) != IS_LONG) {
      zend_error(E_WARNING, "MongoDate:__construct() expects integer values");
      return;
    }

    zend_update_property(mongo_ce_Date, getThis(), "sec", strlen("sec"), arg1 TSRMLS_CC);
    // usec is already 0, if not set above
    break;
  case 0: {
#ifdef WIN32
    time_t sec = time(0);
    zend_update_property_long(mongo_ce_Date, getThis(), "sec", strlen("sec"), sec TSRMLS_CC);
    zend_update_property_long(mongo_ce_Date, getThis(), "usec", strlen("usec"), 0 TSRMLS_CC);
#else
    struct timeval time;
    gettimeofday(&time, NULL);

    zend_update_property_long(mongo_ce_Date, getThis(), "sec", strlen("sec"), time.tv_sec TSRMLS_CC);
    zend_update_property_long(mongo_ce_Date, getThis(), "usec", strlen("usec"), (time.tv_usec/1000)*1000 TSRMLS_CC);
#endif
  }
  }
}
/* }}} */


/* {{{ MongoDate::__toString()
 */
PHP_METHOD(MongoDate, __toString) {
  zval *zsec = zend_read_property( mongo_ce_Date, getThis(), "sec", strlen("sec"), NOISY TSRMLS_CC );
  long sec = Z_LVAL_P( zsec );
  zval *zusec = zend_read_property( mongo_ce_Date, getThis(), "usec", strlen("usec"), NOISY TSRMLS_CC );
  long usec = Z_LVAL_P( zusec );
  double dusec = (double)usec/1000000;
  char *str;

  spprintf(&str, 0, "%.8f %ld", dusec, sec);
  RETURN_STRING(str, 0);
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
  long bin_len, type = 2;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &bin, &bin_len, &type) == FAILURE) {
    return;
  }
  
  zend_update_property_stringl(mongo_ce_BinData, getThis(), "bin", strlen("bin"), bin, bin_len TSRMLS_CC);
  zend_update_property_long(mongo_ce_BinData, getThis(), "type", strlen("type"), type TSRMLS_CC);
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

  // fields
  zend_declare_property_string(mongo_ce_BinData, "bin", strlen("bin"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_BinData, "type", strlen("type"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);
 
  // constants
  // can't use FUNCTION because it's a reserved word
  zend_declare_class_constant_long(mongo_ce_BinData, "FUNC", strlen("FUNC"), 0x01 TSRMLS_CC);
  // can't use ARRAY because it's a reserved word
  zend_declare_class_constant_long(mongo_ce_BinData, "BYTE_ARRAY", strlen("BYTE_ARRAY"), 0x02 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_BinData, "UUID", strlen("UUID"), 0x03 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_BinData, "MD5", strlen("MD5"), 0x05 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_BinData, "CUSTOM", strlen("CUSTOM"), 0x80 TSRMLS_CC);
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
    zend_update_property(mongo_ce_Regex, getThis(), "regex", strlen("regex"), oregex TSRMLS_CC);

    oflags = zend_read_property(mongo_ce_Regex, regex, "flags", strlen("flags"), NOISY TSRMLS_CC);
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

    zend_update_property_stringl(mongo_ce_Regex, getThis(), "regex", strlen("regex"), re+1, pattern_len TSRMLS_CC);
    zend_update_property_stringl(mongo_ce_Regex, getThis(), "flags", strlen("flags"), eopattern, flags_len TSRMLS_CC);
  }
}
/* }}} */


/* {{{ MongoRegex::__toString()
 */
PHP_METHOD(MongoRegex, __toString) {
  char *field_name;
  zval *zre = zend_read_property( mongo_ce_Regex, getThis(), "regex", strlen("regex"), NOISY TSRMLS_CC );
  char *re = Z_STRVAL_P( zre );
  zval *zopts = zend_read_property( mongo_ce_Regex, getThis(), "flags", strlen("regex"), NOISY TSRMLS_CC );
  char *opts = Z_STRVAL_P( zopts );

  spprintf(&field_name, 0, "/%s/%s", re, opts);
  RETVAL_STRING(field_name, 0);
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
  char *code;
  int code_len;
  zval *zcope = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &code, &code_len, &zcope) == FAILURE) {
    return;
  }

  zend_update_property_stringl(mongo_ce_Code, getThis(), "code", strlen("code"), code, code_len TSRMLS_CC);

  if (!zcope) {
    MAKE_STD_ZVAL(zcope);
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
  zval *zode = zend_read_property(mongo_ce_Code, getThis(), "code", strlen("code"), NOISY TSRMLS_CC);
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
 *
 * DB refs are of the form:
 * array( '$ref' => <collection>, '$id' => <id>[, $db => <dbname>] )
 */
PHP_METHOD(MongoDBRef, create) {
  zval *zns, *zid, *zdb = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|z", &zns, &zid, &zdb) == FAILURE) {
    return;
  }

  array_init(return_value);

  // add collection name
  convert_to_string(zns);
  add_assoc_zval(return_value, "$ref", zns); 
  zval_add_ref(&zns);

  // add id field
  add_assoc_zval(return_value, "$id", zid); 
  zval_add_ref(&zid);

  // if we got a database name, add that, too
  if (zdb) {
    convert_to_string(zdb);
    add_assoc_zval(return_value, "$db", zdb);
    zval_add_ref(&zdb);
  }
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

  // check that $ref and $id fields exists
  if (zend_hash_exists(HASH_P(ref), "$ref", strlen("$ref")+1) &&
      zend_hash_exists(HASH_P(ref), "$id", strlen("$id")+1)) {
    // good enough
    RETURN_TRUE;
  }

  RETURN_FALSE;
}
/* }}} */

/* {{{ MongoDBRef::get()
 */
PHP_METHOD(MongoDBRef, get) {
  zval *db, *ref, *collection, *query;
  zval **ns, **id, **dbname;
  zend_bool alloced_db = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &db, mongo_ce_DB, &ref) == FAILURE) {
    return;
  }
  
  if (IS_SCALAR_P(ref) ||
      zend_hash_find(HASH_P(ref), "$ref", strlen("$ref")+1, (void**)&ns) == FAILURE ||
      zend_hash_find(HASH_P(ref), "$id", strlen("$id")+1, (void**)&id) == FAILURE) {
    RETURN_NULL();
  }

  if (Z_TYPE_PP(ns) != IS_STRING) {
      zend_throw_exception(mongo_ce_Exception, "MongoDBRef::get: $ref field must be a string", 0 TSRMLS_CC);
      return;
  }

  // if this reference contains a db name, we have to switch dbs
  if (zend_hash_find(HASH_P(ref), "$db", strlen("$db")+1, (void**)&dbname) == SUCCESS) {
    mongo_db *temp_db = (mongo_db*)zend_object_store_get_object(db TSRMLS_CC);

    // just to be paranoid, make sure dbname is a string
    if (Z_TYPE_PP(dbname) != IS_STRING) {
      zend_throw_exception(mongo_ce_Exception, "MongoDBRef::get: $db field must be a string", 0 TSRMLS_CC);
      return;
    }

    // if the name in the $db field doesn't match the current db, make up a new db
    if (strcmp(Z_STRVAL_PP(dbname), Z_STRVAL_P(temp_db->name)) != 0) {
      zval *new_db_z;
      MAKE_STD_ZVAL(new_db_z);

      MONGO_METHOD1(Mongo, selectDB, new_db_z, temp_db->link, *dbname);

      // make the new db the current one
      db = new_db_z;

      // so we can dtor this later
      alloced_db = 1;
    }
  }

  // get the collection
  MAKE_STD_ZVAL(collection);
  MONGO_METHOD1(MongoDB, selectCollection, collection, db, *ns);
  
  // query for the $id
  MAKE_STD_ZVAL(query);
  array_init(query);
  add_assoc_zval(query, "_id", *id);
  zval_add_ref(id);
  
  // return whatever's there
  MONGO_METHOD1(MongoCollection, findOne, return_value, collection, query);

  // cleanup
  zval_ptr_dtor(&collection);
  zval_ptr_dtor(&query);
  if (alloced_db) {
    zval_ptr_dtor(&db);
  }
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



/* {{{ MongoInt32::__construct(string) 
 */
PHP_METHOD(MongoInt32, __construct) {
  char *value;
  int value_len;
  zval *zcope = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len) == FAILURE) {
    return;
  }

  zend_update_property_stringl(mongo_ce_Int32, getThis(), "value", strlen("value"), value, value_len TSRMLS_CC);
}
/* }}} */


/* {{{ MongoInt32::__toString() 
 */
PHP_METHOD(MongoInt32, __toString ) {
  zval *zode = zend_read_property(mongo_ce_Int32, getThis(), "value", strlen("value"), NOISY TSRMLS_CC);
  RETURN_STRING(Z_STRVAL_P(zode), 1 );
}
/* }}} */


static function_entry MongoInt32_methods[] = {
  PHP_ME(MongoInt32, __construct, NULL, ZEND_ACC_PUBLIC )
  PHP_ME(MongoInt32, __toString, NULL, ZEND_ACC_PUBLIC )
  { NULL, NULL, NULL }
};

void mongo_init_MongoInt32(TSRMLS_D) {
  zend_class_entry ce; 
  INIT_CLASS_ENTRY(ce, "MongoInt32", MongoInt32_methods); 
  mongo_ce_Int32 = zend_register_internal_class(&ce TSRMLS_CC); 

  zend_declare_property_string(mongo_ce_Int32, "value", strlen("value"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
}



/* {{{ MongoInt64::__construct(string) 
 */
PHP_METHOD(MongoInt64, __construct) {
  char *value;
  int value_len;
  zval *zcope = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len) == FAILURE) {
    return;
  }

  zend_update_property_stringl(mongo_ce_Int64, getThis(), "value", strlen("value"), value, value_len TSRMLS_CC);
}
/* }}} */


/* {{{ MongoInt64::__toString() 
 */
PHP_METHOD(MongoInt64, __toString ) {
  zval *zode = zend_read_property(mongo_ce_Int64, getThis(), "value", strlen("value"), NOISY TSRMLS_CC);
  RETURN_STRING(Z_STRVAL_P(zode), 1 );
}
/* }}} */


static function_entry MongoInt64_methods[] = {
  PHP_ME(MongoInt64, __construct, NULL, ZEND_ACC_PUBLIC )
  PHP_ME(MongoInt64, __toString, NULL, ZEND_ACC_PUBLIC )
  { NULL, NULL, NULL }
};

void mongo_init_MongoInt64(TSRMLS_D) {
  zend_class_entry ce; 
  INIT_CLASS_ENTRY(ce, "MongoInt64", MongoInt64_methods); 
  mongo_ce_Int64 = zend_register_internal_class(&ce TSRMLS_CC); 

  zend_declare_property_string(mongo_ce_Int64, "value", strlen("value"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
}
