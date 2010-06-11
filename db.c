// db.c
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


#include <php.h>
#include <zend_exceptions.h>
#include <ext/standard/md5.h>

#include "db.h"
#include "php_mongo.h"
#include "collection.h"
#include "cursor.h"
#include "gridfs.h"
#include "mongo_types.h"

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_Collection,
  *mongo_ce_Cursor,
  *mongo_ce_GridFS,
  *mongo_ce_Id,
  *mongo_ce_Code,
  *mongo_ce_Exception;

extern int le_pconnection,
  le_connection;

extern zend_object_handlers mongo_default_handlers;

zend_class_entry *mongo_ce_DB = NULL;

/* 
 * arginfo needs to be set for __get because if PHP doesn't know it only takes
 * one arg, it will issue a warning.
 */
#if ZEND_MODULE_API_NO < 20090115
static
#endif
ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()


/* {{{ MongoDB::__construct
 */
PHP_METHOD(MongoDB, __construct) {
  zval *zlink;
  char *name;
  int name_len;
  mongo_db *db;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &zlink, mongo_ce_Mongo, &name, &name_len) == FAILURE) {
    return;
  }

  if (0 == name_len ||
      0 != strchr(name, ' ') || 0 != strchr(name, '.') || 0 != strchr(name, '\\') || 
      0 != strchr(name, '/') || 0 != strchr(name, '$')) {
#if ZEND_MODULE_API_NO >= 20060613
    zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "MongoDB::__construct(): invalid name %s", name);
#else
    zend_throw_exception_ex(zend_exception_get_default(), 0 TSRMLS_CC, "MongoDB::__construct(): invalid name %s", name);
#endif /* ZEND_MODULE_API_NO >= 20060613 */
    return;
  }

  db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);

  db->link = zlink;
  zval_add_ref(&db->link);

  MAKE_STD_ZVAL(db->name);
  ZVAL_STRING(db->name, name, 1);
}
/* }}} */

PHP_METHOD(MongoDB, __toString) {
  mongo_db *db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED_STRING(db->name, MongoDB);
  RETURN_ZVAL(db->name, 1, 0);
}

PHP_METHOD(MongoDB, selectCollection) {
  zval temp;
  zval *collection;
  mongo_db *db;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &collection) == FAILURE) {
    return;
  }

  db = (mongo_db*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(db->name, MongoDB);
 
  object_init_ex(return_value, mongo_ce_Collection);

  MONGO_METHOD2(MongoCollection, __construct, &temp, return_value, getThis(), collection); 
}

PHP_METHOD(MongoDB, getGridFS) {
  zval temp;
  zval *arg1 = 0, *arg2 = 0;

  // arg2 is deprecated
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &arg1, &arg2) == FAILURE) {
    return;
  }

  object_init_ex(return_value, mongo_ce_GridFS);

  if (!arg1) {
    MONGO_METHOD1(MongoGridFS, __construct, &temp, return_value, getThis());
  }
  else {
    MONGO_METHOD2(MongoGridFS, __construct, &temp, return_value, getThis(), arg1);
  }
}

PHP_METHOD(MongoDB, getProfilingLevel) {
  zval l;
  Z_TYPE(l) = IS_LONG;
  Z_LVAL(l) = -1;

  MONGO_METHOD1(MongoDB, setProfilingLevel, return_value, getThis(), &l);
}

PHP_METHOD(MongoDB, setProfilingLevel) {
  long level;
  zval *data, *cmd_return;
  zval **ok;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &level) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "profile", level);

  MAKE_STD_ZVAL(cmd_return);
  MONGO_CMD(cmd_return, getThis());

  zval_ptr_dtor(&data);

  if (EG(exception)) {
    zval_ptr_dtor(&cmd_return);
    return;
  }

  if (zend_hash_find(HASH_P(cmd_return), "ok", 3, (void**)&ok) == SUCCESS &&
      ((Z_TYPE_PP(ok) == IS_BOOL && Z_BVAL_PP(ok)) || Z_DVAL_PP(ok) == 1)) {
    zend_hash_find(HASH_P(cmd_return), "was", 4, (void**)&ok);
    RETVAL_ZVAL(*ok, 1, 0);
  }
  else {
    RETVAL_NULL();
  }
  zval_ptr_dtor(&cmd_return);
}

PHP_METHOD(MongoDB, drop) {
  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "dropDatabase", 1);

  MONGO_CMD(return_value, getThis());
  zval_ptr_dtor(&data);
}

PHP_METHOD(MongoDB, repair) {
  zend_bool cloned=0, original=0;
  zval *data;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|bb", &cloned, &original) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "repairDatabase", 1);
  add_assoc_bool(data, "preserveClonedFilesOnFailure", cloned);
  add_assoc_bool(data, "backupOriginalFiles", original);

  MONGO_CMD(return_value, getThis());

  zval_ptr_dtor(&data);
}


PHP_METHOD(MongoDB, createCollection) {
  zval *collection, *data, *temp;
  zend_bool capped=0;
  int size=0, max=0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|bll", &collection, &capped, &size, &max) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  convert_to_string(collection);
  add_assoc_zval(data, "create", collection);
  zval_add_ref(&collection);

  if (size) {
    add_assoc_long(data, "size", size);
  }

  if (capped) {
    add_assoc_bool(data, "capped", 1);
    if (max) {
      add_assoc_long(data, "max", max);
    }
  }

  MAKE_STD_ZVAL(temp);
  MONGO_CMD(temp, getThis());
  zval_ptr_dtor(&temp);

  zval_ptr_dtor(&data);

  // get the collection we just created
  MONGO_METHOD1(MongoDB, selectCollection, return_value, getThis(), collection);
}

PHP_METHOD(MongoDB, dropCollection) {
  zval *collection;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &collection) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(collection) != IS_OBJECT ||
      Z_OBJCE_P(collection) != mongo_ce_Collection) {
    zval *temp;

    MAKE_STD_ZVAL(temp);
    MONGO_METHOD1(MongoDB, selectCollection, temp, getThis(), collection);
    collection = temp;
  }
  else {
    zval_add_ref(&collection);
  }

  MONGO_METHOD(MongoCollection, drop, return_value, collection);

  zval_ptr_dtor(&collection);
}

PHP_METHOD(MongoDB, listCollections) {
  // select db.system.namespaces collection
  zval *nss, *collection, *cursor, *list, *next;

  MAKE_STD_ZVAL(nss);
  ZVAL_STRING(nss, "system.namespaces", 1);

  MAKE_STD_ZVAL(collection);
  MONGO_METHOD1(MongoDB, selectCollection, collection, getThis(), nss);
  
  // list to return
  MAKE_STD_ZVAL(list);
  array_init(list);

  // do find  
  MAKE_STD_ZVAL(cursor);
  MONGO_METHOD(MongoCollection, find, cursor, collection);
 
  // populate list
  MAKE_STD_ZVAL(next);  
  MONGO_METHOD(MongoCursor, getNext, next, cursor);
  while (Z_TYPE_P(next) != IS_NULL) {
    zval *c, *zname;
    zval **collection;
    char *name, *first_dot, *system;

    // check that the ns is valid and not an index (contains $)
    if (zend_hash_find(HASH_P(next), "name", 5, (void**)&collection) == FAILURE ||
        strchr(Z_STRVAL_PP(collection), '$')) {

      zval_ptr_dtor(&next);
      MAKE_STD_ZVAL(next);

      MONGO_METHOD(MongoCursor, getNext, next, cursor);
      continue;
    }

    first_dot = strchr(Z_STRVAL_PP(collection), '.');
    system = strstr(Z_STRVAL_PP(collection), ".system.");
    // check that this isn't a system ns
    if ((system && first_dot == system) ||
	(name = strchr(Z_STRVAL_PP(collection), '.')) == 0) {

      zval_ptr_dtor(&next);
      MAKE_STD_ZVAL(next);

      MONGO_METHOD(MongoCursor, getNext, next, cursor);
      continue;
    }

    // take a substring after the first "."
    name++;

    MAKE_STD_ZVAL(c);

    MAKE_STD_ZVAL(zname);
    // name must be copied because it is a substring of
    // a string that will be garbage collected in a sec
    ZVAL_STRING(zname, name, 1);
    MONGO_METHOD1(MongoDB, selectCollection, c, getThis(), zname);

    add_next_index_zval(list, c);

    zval_ptr_dtor(&zname);
    zval_ptr_dtor(&next);
    MAKE_STD_ZVAL(next);

    MONGO_METHOD(MongoCursor, getNext, next, cursor);
  }

  zval_ptr_dtor(&next);
  zval_ptr_dtor(&nss);
  zval_ptr_dtor(&cursor);
  zval_ptr_dtor(&collection);

  RETURN_ZVAL(list, 0, 1);
}

PHP_METHOD(MongoDB, createDBRef) {
  zval *ns, *obj;
  zval **id;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &ns, &obj) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(obj) == IS_ARRAY ||
      Z_TYPE_P(obj) == IS_OBJECT) {
    if (zend_hash_find(HASH_P(obj), "_id", 4, (void**)&id) == SUCCESS) {
      MONGO_METHOD2(MongoDBRef, create, return_value, NULL, ns, *id);
      return;
    }
    else if (Z_TYPE_P(obj) == IS_ARRAY) {
      return;
    }
  }

  MONGO_METHOD2(MongoDBRef, create, return_value, NULL, ns, obj);
}

PHP_METHOD(MongoDB, getDBRef) {
  zval *ref;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ref) == FAILURE) {
    return;
  }

  MONGO_METHOD2(MongoDBRef, get, return_value, NULL, getThis(), ref);
}

PHP_METHOD(MongoDB, execute) {
  zval *code = 0, *args = 0, *zdata;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &code, &args) == FAILURE) {
    return;
  }
  
  if (!args) {
    MAKE_STD_ZVAL(args);
    array_init(args);
  }
  else {
    zval_add_ref(&args);
  }

  // turn the first argument into MongoCode
  if (Z_TYPE_P(code) != IS_OBJECT || 
      Z_OBJCE_P(code) != mongo_ce_Code) {
    zval *obj;

    MAKE_STD_ZVAL(obj);
    object_init_ex(obj, mongo_ce_Code);
    MONGO_METHOD1(MongoCode, __construct, return_value, obj, code);
    code = obj;
  }
  else {
    zval_add_ref(&code);
  }

  // create { $eval : code, args : [] }
  MAKE_STD_ZVAL(zdata);
  array_init(zdata);
  add_assoc_zval(zdata, "$eval", code);
  add_assoc_zval(zdata, "args", args);

  MONGO_METHOD1(MongoDB, command, return_value, getThis(), zdata);

  zval_ptr_dtor(&zdata);
}

static char *get_cmd_ns(char *db, int db_len) {
  char *position;

  char *cmd_ns = (char*)emalloc(db_len + strlen("$cmd") + 2);
  position = cmd_ns;

  // db
  memcpy(position, db, db_len);
  position += db_len;

  // .
  *(position)++ = '.';

  // $cmd
  memcpy(position, "$cmd", strlen("$cmd"));
  position += strlen("$cmd");

  // \0
  *(position) = '\0';

  return cmd_ns;
}

PHP_METHOD(MongoDB, command) {
  zval temp, limit;
  zval *cmd, *cursor, *ns;
  mongo_db *db;
  char *cmd_ns;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &cmd) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(cmd)) {
    zend_error(E_WARNING, "MongoDB::command() expects parameter 1 to be an array or object");
    return;
  }

  PHP_MONGO_GET_DB(getThis());

  // create db.$cmd
  MAKE_STD_ZVAL(ns);
  cmd_ns = get_cmd_ns(Z_STRVAL_P(db->name), Z_STRLEN_P(db->name));
  ZVAL_STRING(ns, cmd_ns, 0);

  // create cursor
  MAKE_STD_ZVAL(cursor);
  object_init_ex(cursor, mongo_ce_Cursor);
  MONGO_METHOD3(MongoCursor, __construct, &temp, cursor, db->link, ns, cmd); 

  zval_ptr_dtor(&ns);

  // limit
  Z_TYPE(limit) = IS_LONG;
  Z_LVAL(limit) = -1;

  MONGO_METHOD1(MongoCursor, limit, &temp, cursor, &limit);

  // query
  MONGO_METHOD(MongoCursor, getNext, return_value, cursor);

  zend_objects_store_del_ref(cursor TSRMLS_CC);
  zval_ptr_dtor(&cursor);
}

static void md5_hash(char *md5str, char *arg) {
  PHP_MD5_CTX context;
  unsigned char digest[16];
  	
  md5str[0] = '\0';
  PHP_MD5Init(&context);
  PHP_MD5Update(&context, arg, strlen(arg));
  PHP_MD5Final(digest, &context);
  make_digest(md5str, digest);
}


PHP_METHOD(MongoDB, authenticate) {
  char *username, *password;
  int ulen, plen;
  zval *data, *result, **nonce;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &username, &ulen, &password, &plen) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "getnonce", 1);

  MAKE_STD_ZVAL(result);
  MONGO_CMD(result, getThis());

  zval_ptr_dtor(&data);

  if (zend_hash_find(HASH_P(result), "nonce", strlen("nonce")+1, (void**)&nonce) == SUCCESS) {
    char *salt, *rash;
    char hash[33], digest[33];

    // create username:mongo:password hash
    spprintf(&salt, 0, "%s:mongo:%s", username, password);
    md5_hash(hash, salt);
    efree(salt);

    // create nonce|username|hash hash
    spprintf(&rash, 0, "%s%s%s", Z_STRVAL_PP(nonce), username, hash);
    md5_hash(digest, rash);
    efree(rash);

    // make actual authentication cmd
    MAKE_STD_ZVAL(data);
    array_init(data);

    add_assoc_long(data, "authenticate", 1);
    add_assoc_stringl(data, "user", username, ulen, 1);
    add_assoc_zval(data, "nonce", *nonce);
    zval_add_ref(nonce);
    add_assoc_string(data, "key", digest, 1);

    MONGO_CMD(return_value, getThis());

    zval_ptr_dtor(&data);
  }
  else {
    RETVAL_FALSE;
  }

  zval_ptr_dtor(&result);
}


static void run_err(char *cmd, zval *return_value, zval *db TSRMLS_DC) {
  zval *data;
  
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, cmd, 1);
  
  MONGO_CMD(return_value, db);
  
  zval_ptr_dtor(&data);
}

/* {{{ MongoDB->lastError()
 */
PHP_METHOD(MongoDB, lastError) {
  run_err("getlasterror", return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ MongoDB->prevError()
 */
PHP_METHOD(MongoDB, prevError) {
  run_err("getpreverror", return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ MongoDB->resetError()
 */
PHP_METHOD(MongoDB, resetError) {
  run_err("reseterror", return_value, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ MongoDB->forceError()
 */
PHP_METHOD(MongoDB, forceError) {
  run_err("forceerror", return_value, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ MongoDB::__get
 */
PHP_METHOD(MongoDB, __get) {
  zval *name;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
    return;
  }

  // select this collection
  MONGO_METHOD1(MongoDB, selectCollection, return_value, getThis(), name);
}
/* }}} */



static function_entry MongoDB_methods[] = {
  PHP_ME(MongoDB, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, __toString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, __get, arginfo___get, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getGridFS, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getProfilingLevel, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, setProfilingLevel, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, drop, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, repair, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, selectCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, createCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, dropCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, listCollections, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, createDBRef, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, getDBRef, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, execute, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, command, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, lastError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, prevError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, resetError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, forceError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoDB, authenticate, NULL, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};

static void php_mongo_db_free(void *object TSRMLS_DC) {
  mongo_db *db = (mongo_db*)object;

  if (db) {
    if (db->link) {
      zval_ptr_dtor(&db->link);
    }
    if (db->name) {
      zval_ptr_dtor(&db->name);
    }

    zend_object_std_dtor(&db->std TSRMLS_CC);
    efree(db);
  }
}


/* {{{ mongo_mongo_db_new
 */
zend_object_value php_mongo_db_new(zend_class_entry *class_type TSRMLS_DC) {
  php_mongo_obj_new(mongo_db);
}
/* }}} */


void mongo_init_MongoDB(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoDB", MongoDB_methods);
  ce.create_object = php_mongo_db_new;
  mongo_ce_DB = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_OFF", strlen("PROFILING_OFF"), 0 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_SLOW", strlen("PROFILING_SLOW"), 1 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_DB, "PROFILING_ON", strlen("PROFILING_ON"), 2 TSRMLS_CC);

  zend_declare_property_long(mongo_ce_DB, "w", strlen("w"), 1, ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_DB, "wtimeout", strlen("wtimeout"), PHP_MONGO_DEFAULT_TIMEOUT, ZEND_ACC_PUBLIC TSRMLS_CC);
}
