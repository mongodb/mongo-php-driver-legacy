// php_mongo.c
/**
 *  Copyright 2009-2011 10gen, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

#include <zend_exceptions.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "php_mongo.h"

#include "mongo.h"
#include "cursor.h"
#include "mongo_types.h"

#include "util/pool.h"
#include "util/server.h"
#include "util/rs.h"
#include "util/log.h"

extern zend_object_handlers mongo_default_handlers,
  mongo_id_handlers;

/** Classes */
extern zend_class_entry *mongo_ce_CursorException;

zend_class_entry *mongo_ce_ConnectionException,
  *mongo_ce_CursorTOException,
  *mongo_ce_GridFSException,
  *mongo_ce_Exception,
  *mongo_ce_MaxKey,
  *mongo_ce_MinKey;

/** Resources */
int le_pconnection,
  le_pserver,
  le_prs,
  le_cursor_list;

static void mongo_init_MongoExceptions(TSRMLS_D);

ZEND_DECLARE_MODULE_GLOBALS(mongo)

#if ZEND_MODULE_API_NO >= 20060613
// 5.2+ globals
static PHP_GINIT_FUNCTION(mongo);
#else
// 5.1- globals
static void mongo_init_globals(zend_mongo_globals* g TSRMLS_DC);
#endif /* ZEND_MODULE_API_NO >= 20060613 */

#if WIN32
extern HANDLE cursor_mutex;
extern HANDLE io_mutex;
extern HANDLE pool_mutex;
#endif

zend_function_entry mongo_functions[] = {
  PHP_FE(bson_encode, NULL)
  PHP_FE(bson_decode, NULL)
  { NULL, NULL, NULL }
};

/* {{{ mongo_module_entry
 */
zend_module_entry mongo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  PHP_MONGO_EXTNAME,
  mongo_functions,
  PHP_MINIT(mongo),
  PHP_MSHUTDOWN(mongo),
  PHP_RINIT(mongo),
  NULL,
  PHP_MINFO(mongo),
  PHP_MONGO_VERSION,
#if ZEND_MODULE_API_NO >= 20060613
  PHP_MODULE_GLOBALS(mongo),
  PHP_GINIT(mongo),
  NULL,
  NULL,
  STANDARD_MODULE_PROPERTIES_EX
#else
  STANDARD_MODULE_PROPERTIES
#endif
};
/* }}} */

#ifdef COMPILE_DL_MONGO
ZEND_GET_MODULE(mongo)
#endif


/* {{{ PHP_INI */
// these must be in the same order as mongo_globals are declared or it will segfault on 64-bit machines!
PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("mongo.auto_reconnect", "1", PHP_INI_ALL, OnUpdateLong, auto_reconnect, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.allow_persistent", "1", PHP_INI_ALL, OnUpdateLong, allow_persistent, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.default_host", "localhost", PHP_INI_ALL, OnUpdateString, default_host, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.default_port", "27017", PHP_INI_ALL, OnUpdateLong, default_port, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.chunk_size", "262144", PHP_INI_ALL, OnUpdateLong, chunk_size, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.cmd", "$", PHP_INI_ALL, OnUpdateStringUnempty, cmd_char, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.utf8", "1", PHP_INI_ALL, OnUpdateLong, utf8, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.native_long", "0", PHP_INI_ALL, OnUpdateLong, native_long, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.long_as_object", "0", PHP_INI_ALL, OnUpdateLong, long_as_object, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.allow_empty_keys", "0", PHP_INI_ALL, OnUpdateLong, allow_empty_keys, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.no_id", "0", PHP_INI_SYSTEM, OnUpdateLong, no_id, zend_mongo_globals, mongo_globals)
PHP_INI_END()
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mongo) {
  zend_class_entry max_key, min_key;

#if ZEND_MODULE_API_NO < 20060613
  ZEND_INIT_MODULE_GLOBALS(mongo, mongo_init_globals, NULL);
#endif /* ZEND_MODULE_API_NO < 20060613 */

  REGISTER_INI_ENTRIES();

  le_pconnection = zend_register_list_destructors_ex(NULL, mongo_util_pool_shutdown, PHP_CONNECTION_RES_NAME, module_number);
  le_pserver = zend_register_list_destructors_ex(NULL, mongo_util_server_shutdown, PHP_SERVER_RES_NAME, module_number);
  le_prs = zend_register_list_destructors_ex(NULL, mongo_util_rs_shutdown, PHP_RS_RES_NAME, module_number);
  le_cursor_list = zend_register_list_destructors_ex(NULL, php_mongo_cursor_list_pfree, PHP_CURSOR_LIST_RES_NAME, module_number);

  mongo_init_Mongo(TSRMLS_C);
  mongo_init_MongoDB(TSRMLS_C);
  mongo_init_MongoCollection(TSRMLS_C);
  mongo_init_MongoCursor(TSRMLS_C);

  mongo_init_MongoGridFS(TSRMLS_C);
  mongo_init_MongoGridFSFile(TSRMLS_C);
  mongo_init_MongoGridFSCursor(TSRMLS_C);

  mongo_init_MongoId(TSRMLS_C);
  mongo_init_MongoCode(TSRMLS_C);
  mongo_init_MongoRegex(TSRMLS_C);
  mongo_init_MongoDate(TSRMLS_C);
  mongo_init_MongoBinData(TSRMLS_C);
  mongo_init_MongoDBRef(TSRMLS_C);

  mongo_init_MongoExceptions(TSRMLS_C);

  mongo_init_MongoTimestamp(TSRMLS_C);
  mongo_init_MongoInt32(TSRMLS_C);
  mongo_init_MongoInt64(TSRMLS_C);

  mongo_init_MongoLog(TSRMLS_C);
  mongo_init_MongoPool(TSRMLS_C);

  /*
   * MongoMaxKey and MongoMinKey are completely non-interactive: they have no
   * method, fields, or constants.
   */
  INIT_CLASS_ENTRY(max_key, "MongoMaxKey", NULL);
  mongo_ce_MaxKey = zend_register_internal_class(&max_key TSRMLS_CC);
  INIT_CLASS_ENTRY(min_key, "MongoMinKey", NULL);
  mongo_ce_MinKey = zend_register_internal_class(&min_key TSRMLS_CC);


  // make mongo objects uncloneable
  memcpy(&mongo_default_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mongo_default_handlers.clone_obj = NULL;

  // add compare_objects for MongoId
  memcpy(&mongo_id_handlers, &mongo_default_handlers, sizeof(zend_object_handlers));
  mongo_id_handlers.compare_objects = php_mongo_compare_ids;

  // start random number generator
  srand(time(0));

#ifdef WIN32
  cursor_mutex = CreateMutex(NULL, FALSE, NULL);
  pool_mutex = CreateMutex(NULL, FALSE, NULL);
  io_mutex = CreateMutex(NULL, FALSE, NULL);
  if (cursor_mutex == NULL || pool_mutex == NULL || io_mutex == NULL) {
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Windows couldn't create a mutex: %s", GetLastError());
    return FAILURE;
  }
#endif

  return SUCCESS;
}


#if ZEND_MODULE_API_NO >= 20060613
/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mongo)
#else
/* {{{ mongo_init_globals
 */
static void mongo_init_globals(zend_mongo_globals *mongo_globals TSRMLS_DC)
#endif /* ZEND_MODULE_API_NO >= 20060613 */
{
  // on windows, the max length is 256.  linux doesn't have a limit, but it will
  // fill in the first 256 chars of hostname even if the actual hostname is
  // longer.  if you can't get a unique character in the first 256 chars of your
  // hostname, you're doing it wrong.
  int len, win_max = 256;
  char *hostname, host_start[256];
  register ulong hash;

  mongo_globals->auto_reconnect = 1;
  mongo_globals->default_host = "localhost";
  mongo_globals->default_port = 27017;
  mongo_globals->request_id = 3;
  mongo_globals->chunk_size = DEFAULT_CHUNK_SIZE;
  mongo_globals->cmd_char = "$";
  mongo_globals->utf8 = 1;

  mongo_globals->inc = 0;
  mongo_globals->response_num = 0;
  mongo_globals->errmsg = 0;

  mongo_globals->max_send_size = 64 * 1024 * 1024;
  mongo_globals->pool_size = -1;

  hostname = host_start;
  // from the gnu manual:
  //     gethostname stores the beginning of the host name in name even if the
  //     host name won't entirely fit. For some purposes, a truncated host name
  //     is good enough. If it is, you can ignore the error code.
  // so we'll ignore the error code.
  // returns 0-terminated hostname.
  gethostname(hostname, win_max);
  len = strlen(hostname);

  hash = 5381;

  /* from zend_hash.h */
  /* variant with the hash unrolled eight times */
  for (; len >= 8; len -= 8) {
    hash = ((hash << 5) + hash) + *hostname++;
    hash = ((hash << 5) + hash) + *hostname++;
    hash = ((hash << 5) + hash) + *hostname++;
    hash = ((hash << 5) + hash) + *hostname++;
    hash = ((hash << 5) + hash) + *hostname++;
    hash = ((hash << 5) + hash) + *hostname++;
    hash = ((hash << 5) + hash) + *hostname++;
    hash = ((hash << 5) + hash) + *hostname++;
  }

  switch (len) {
  case 7: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
  case 6: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
  case 5: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
  case 4: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
  case 3: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
  case 2: hash = ((hash << 5) + hash) + *hostname++; /* fallthrough... */
  case 1: hash = ((hash << 5) + hash) + *hostname++; break;
  case 0: break;
  }

  mongo_globals->machine = hash;

  mongo_globals->ts_inc = 0;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(mongo) {
  UNREGISTER_INI_ENTRIES();

#if WIN32
  // 0 is failure
  if (CloseHandle(cursor_mutex) == 0 || CloseHandle(pool_mutex) == 0 || CloseHandle(io_mutex) == 0) {
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Windows couldn't destroy a mutex: %s", GetLastError());
    return FAILURE;
  }
#endif

  return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(mongo) {
  return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mongo) {
  php_info_print_table_start();

  php_info_print_table_header(2, "MongoDB Support", "enabled");
  php_info_print_table_row(2, "Version", PHP_MONGO_VERSION);

  php_info_print_table_end();

  DISPLAY_INI_ENTRIES();
}
/* }}} */

static void mongo_init_MongoExceptions(TSRMLS_D) {
  zend_class_entry e, conn, e2, ctoe;

  INIT_CLASS_ENTRY(e, "MongoException", NULL);

#if ZEND_MODULE_API_NO >= 20060613
  mongo_ce_Exception = zend_register_internal_class_ex(&e, (zend_class_entry*)zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
#else
  mongo_ce_Exception = zend_register_internal_class_ex(&e, (zend_class_entry*)zend_exception_get_default(), NULL TSRMLS_CC);
#endif /* ZEND_MODULE_API_NO >= 20060613 */

  mongo_init_CursorExceptions(TSRMLS_C);

  INIT_CLASS_ENTRY(ctoe, "MongoCursorTimeoutException", NULL);
  mongo_ce_CursorTOException = zend_register_internal_class_ex(&ctoe, mongo_ce_CursorException, NULL TSRMLS_CC);

  INIT_CLASS_ENTRY(conn, "MongoConnectionException", NULL);
  mongo_ce_ConnectionException = zend_register_internal_class_ex(&conn, mongo_ce_Exception, NULL TSRMLS_CC);

  INIT_CLASS_ENTRY(e2, "MongoGridFSException", NULL);
  mongo_ce_GridFSException = zend_register_internal_class_ex(&e2, mongo_ce_Exception, NULL TSRMLS_CC);
}
