// mongo.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef WIN32
#include <sys/types.h>
#include <pthread.h>
#endif

#include <php.h>
#include <zend_exceptions.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "php_mongo.h"
#include "db.h"
#include "cursor.h"
#include "mongo_types.h"
#include "bson.h"
#include "util/hash.h"
#include "util/connect.h"
#include "util/pool.h"
#include "util/link.h"
#include "util/rs.h"

extern zend_class_entry *mongo_ce_DB,
  *mongo_ce_CursorException,
  *mongo_ce_CursorTOException,
  *mongo_ce_Cursor,
  *mongo_ce_Id,
  *mongo_ce_Date,
  *mongo_ce_Regex,
  *mongo_ce_Code,
  *mongo_ce_BinData,
  *mongo_ce_Timestamp,
  *mongo_ce_Int32,
  *mongo_ce_Int64;

static void php_mongo_link_free(void* TSRMLS_DC);
static void php_mongo_cursor_list_pfree(zend_rsrc_list_entry* TSRMLS_DC);
static char* php_mongo_get_host(char** current, int domain_socket);
static int php_mongo_get_port(char**);
static void mongo_init_MongoExceptions(TSRMLS_D);
static void run_err(int, zval*, zval* TSRMLS_DC);
static int php_mongo_parse_server(zval *this_ptr TSRMLS_DC);
static char* stringify_server(mongo_server*, char*, int*, int*);
static int get_cursor_body(int sock, mongo_cursor *cursor TSRMLS_DC);
static mongo_cursor* make_persistent_cursor(mongo_cursor *cursor);
static void make_unpersistent_cursor(mongo_cursor *pcursor, mongo_cursor *cursor);

#if WIN32
static HANDLE cursor_mutex;
#else
static pthread_mutex_t cursor_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

zend_object_handlers mongo_default_handlers,
  mongo_id_handlers;

/** Classes */
zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_CursorException,
  *mongo_ce_ConnectionException,
  *mongo_ce_CursorTOException,
  *mongo_ce_GridFSException,
  *mongo_ce_Exception,
  *mongo_ce_MaxKey,
  *mongo_ce_MinKey;

/** Resources */
int le_pconnection,
  le_cursor_list;

ZEND_DECLARE_MODULE_GLOBALS(mongo)

#if ZEND_MODULE_API_NO >= 20060613
// 5.2+ globals
static PHP_GINIT_FUNCTION(mongo);
#else
// 5.1- globals
static void mongo_init_globals(zend_mongo_globals* g TSRMLS_DC);
#endif /* ZEND_MODULE_API_NO >= 20060613 */


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


function_entry mongo_functions[] = {
  PHP_FE(bson_encode, NULL)
  PHP_FE(bson_decode, NULL)
  PHP_FE(mongoPoolDebug, NULL)
  { NULL, NULL, NULL }
};

static function_entry mongo_methods[] = {
  PHP_ME(Mongo, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, connect, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, pairConnect, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, persistConnect, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, pairPersistConnect, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, connectUtil, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Mongo, __toString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, __get, arginfo___get, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, selectDB, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, selectCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, getSlaveOkay, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, setSlaveOkay, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, dropDB, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, lastError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, prevError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, resetError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, forceError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, listDBs, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, getHosts, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, getSlave, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, switchSlave, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, close, NULL, ZEND_ACC_PUBLIC)
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


void php_mongo_server_free(mongo_server *server TSRMLS_DC) {
  // return this connection to the pool
  mongo_util_pool_done(server TSRMLS_CC);

  if (server->host) {
    efree(server->host);
    server->host = 0;
  }
  if (server->label) {
    efree(server->label);
    server->label = 0;
  }
  if (server->username) {
    efree(server->username);
    server->username = 0;
  }
  if (server->password) {
    efree(server->password);
    server->password = 0;
  }
  if (server->db) {
    efree(server->db);
    server->db = 0;
  }

  efree(server);
}

static void php_mongo_server_set_free(mongo_server_set *server_set TSRMLS_DC) {
  mongo_server *current;

  if (!server_set) {
    return;
  }

  current = server_set->server;

  while (current) {
    mongo_server *temp = current->next;
    php_mongo_server_free(current TSRMLS_CC);
    current = temp;
  }

  efree(server_set);
}

// tell db to destroy its cursor
static void kill_cursor(cursor_node *node, list_entry *le TSRMLS_DC) {
  mongo_cursor *cursor = node->cursor;
  char quickbuf[128];
  buffer buf;
  zval temp;

  /*
   * If the cursor_id is 0, the db is out of results anyway.
   */
  if (cursor->cursor_id == 0) {
    php_mongo_free_cursor_node(node, le);
    return;
  }

  buf.pos = quickbuf;
  buf.start = buf.pos;
  buf.end = buf.start + 128;

  php_mongo_write_kill_cursors(&buf, cursor TSRMLS_CC);

  if (!cursor->server) {
    return;
  }
  Z_TYPE(temp) = IS_NULL;
  _mongo_say(cursor->server->socket, &buf, &temp TSRMLS_CC);
  if (Z_TYPE(temp) == IS_STRING) {
    efree(Z_STRVAL(temp));
    Z_TYPE(temp) = IS_NULL;
  }

  /*
   * if the connection is closed before the cursor is destroyed, the cursor
   * might try to fetch more results with disasterous consequences.  Thus, the
   * cursor_id is set to 0, so no more results will be fetched.
   *
   * this might not be the most elegant solution, since you could fetch 100
   * results, get the first one, close the connection, get 99 more, and suddenly
   * not be able to get any more.  Not sure if there's a better one, though. I
   * guess the user can call dead() on the cursor.
   */
  cursor->cursor_id = 0;

  // free this cursor/link pair
  php_mongo_free_cursor_node(node, le);
}

void php_mongo_free_cursor_node(cursor_node *node, list_entry *le) {

  /*
   * [node1][<->][NODE2][<->][node3]
   *   [node1][->][node3]
   *   [node1][<->][node3]
   *
   * [node1][<->][NODE2]
   *   [node1]
   */
  if (node->prev) {
    node->prev->next = node->next;
    if (node->next) {
      node->next->prev = node->prev;
    }
  }
  /*
   * [NODE2][<->][node3]
   *   le->ptr = node3
   *   [node3]
   *
   * [NODE2]
   *   le->ptr = 0
   */
  else {
    le->ptr = node->next;
    if (node->next) {
      node->next->prev = 0;
    }
  }

  pefree(node, 1);
}

int php_mongo_free_cursor_le(void *val, int type TSRMLS_DC) {
  list_entry *le;

  LOCK;

  /*
   * This should work if le->ptr is null or non-null
   */
  if (zend_hash_find(&EG(persistent_list), "cursor_list", strlen("cursor_list") + 1, (void**)&le) == SUCCESS) {
    cursor_node *current;

    current = le->ptr;

    while (current) {
      cursor_node *next = current->next;

      if (type == MONGO_LINK) {
        if (current->cursor->link == (mongo_link*)val) {
          kill_cursor(current, le TSRMLS_CC);
          // keep going, free all cursor for this connection
        }
      }
      else if (type == MONGO_CURSOR) {
        if (current->cursor == (mongo_cursor*)val) {
          kill_cursor(current, le TSRMLS_CC);
          // only one cursor to be freed
          break;
        }
      }

      current = next;
    }
  }

  UNLOCK;
  return 0;
}

int php_mongo_create_le(mongo_cursor *cursor, char *name TSRMLS_DC) {
  list_entry *le;
  cursor_node *new_node;

  LOCK;

  new_node = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
  new_node->cursor = cursor;
  new_node->next = new_node->prev = 0;

  /*
   * 3 options:
   *   - le doesn't exist
   *   - le exists and is null
   *   - le exists and has elements
   * In case 1 & 2, we want to create a new le ptr, otherwise we want to append
   * to the existing ptr.
   */
  if (zend_hash_find(&EG(persistent_list), name, strlen(name)+1, (void**)&le) == SUCCESS) {
    cursor_node *current = le->ptr;
    cursor_node *prev = 0;

    if (current == 0) {
      le->ptr = new_node;
      UNLOCK;
      return 0;
    }

    do {
      /*
       * if we find the current cursor in the cursor list, we don't need another
       * dtor for it so unlock the mutex & return.
       */
      if (current->cursor == cursor) {
        pefree(new_node, 1);
        UNLOCK;
        return 0;
      }

      prev = current;
      current = current->next;
    }
    while (current);

    /*
     * we didn't find the cursor.  add it to the list. (prev is pointing to the
     * tail of the list, current is pointing to null.
     */
    prev->next = new_node;
    new_node->prev = prev;
  }
  else {
    list_entry new_le;
    new_le.ptr = new_node;
    new_le.type = le_cursor_list;
    new_le.refcount = 1;
    zend_hash_add(&EG(persistent_list), name, strlen(name)+1, &new_le, sizeof(list_entry), NULL);
  }

  UNLOCK;
  return 0;
}


/* {{{ php_mongo_link_free
 */
static void php_mongo_link_free(void *object TSRMLS_DC) {
  mongo_link *link = (mongo_link*)object;

  // already freed
  if (!link) {
    return;
  }

  php_mongo_free_cursor_le(link, MONGO_LINK TSRMLS_CC);
  php_mongo_server_set_free(link->server_set TSRMLS_CC);

  if (link->username) efree(link->username);
  if (link->password) efree(link->password);
  if (link->db) efree(link->db);

  zend_object_std_dtor(&link->std TSRMLS_CC);

  efree(link);
}
/* }}} */


static int cursor_list_pfree_helper(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  LOCK;

  {
    cursor_node *node = (cursor_node*)rsrc->ptr;

    if (!node) {
      UNLOCK;
      return 0;
    }

    while (node->next) {
      cursor_node *temp = node;
      node = node->next;

      pefree(temp->cursor->buf.start, 1);
      pefree(temp->cursor, 1);
      pefree(temp, 1);
    }
    pefree(node->cursor->buf.start, 1);
    pefree(node->cursor, 1);
    pefree(node, 1);
  }

  UNLOCK;
  return 0;
}

static void php_mongo_cursor_list_pfree(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  cursor_list_pfree_helper(rsrc TSRMLS_CC);
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mongo) {
  zend_class_entry max_key, min_key;

#if ZEND_MODULE_API_NO < 20060613
  ZEND_INIT_MODULE_GLOBALS(mongo, mongo_init_globals, NULL);
#endif /* ZEND_MODULE_API_NO < 20060613 */

  REGISTER_INI_ENTRIES();

  le_pconnection = zend_register_list_destructors_ex(NULL, mongo_util_pool_shutdown, PHP_CONNECTION_RES_NAME, module_number);
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
  if (cursor_mutex == NULL) {
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

  mongo_globals->max_doc_size = 4 * 1024 * 1024;
  mongo_globals->max_send_size = 64 * 1024 * 1024;

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
  if (CloseHandle(cursor_mutex) == 0) {
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

void mongo_init_MongoExceptions(TSRMLS_D) {
  zend_class_entry e, ce, conn, e2, ctoe;

  INIT_CLASS_ENTRY(e, "MongoException", NULL);

#if ZEND_MODULE_API_NO >= 20060613
  mongo_ce_Exception = zend_register_internal_class_ex(&e, (zend_class_entry*)zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
#else
  mongo_ce_Exception = zend_register_internal_class_ex(&e, (zend_class_entry*)zend_exception_get_default(), NULL TSRMLS_CC);
#endif /* ZEND_MODULE_API_NO >= 20060613 */

  INIT_CLASS_ENTRY(ce, "MongoCursorException", NULL);
  mongo_ce_CursorException = zend_register_internal_class_ex(&ce, mongo_ce_Exception, NULL TSRMLS_CC);

  INIT_CLASS_ENTRY(ctoe, "MongoCursorTimeoutException", NULL);
  mongo_ce_CursorTOException = zend_register_internal_class_ex(&ctoe, mongo_ce_CursorException, NULL TSRMLS_CC);

  INIT_CLASS_ENTRY(conn, "MongoConnectionException", NULL);
  mongo_ce_ConnectionException = zend_register_internal_class_ex(&conn, mongo_ce_Exception, NULL TSRMLS_CC);

  INIT_CLASS_ENTRY(e2, "MongoGridFSException", NULL);
  mongo_ce_GridFSException = zend_register_internal_class_ex(&e2, mongo_ce_Exception, NULL TSRMLS_CC);
}


/* {{{ php_mongo_link_new
 */
static zend_object_value php_mongo_link_new(zend_class_entry *class_type TSRMLS_DC) {
  php_mongo_obj_new(mongo_link);
}
/* }}} */

void mongo_init_Mongo(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "Mongo", mongo_methods);
  ce.create_object = php_mongo_link_new;
  mongo_ce_Mongo = zend_register_internal_class(&ce TSRMLS_CC);

  /* Mongo class constants */
  zend_declare_class_constant_string(mongo_ce_Mongo, "DEFAULT_HOST", strlen("DEFAULT_HOST"), "localhost" TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Mongo, "DEFAULT_PORT", strlen("DEFAULT_PORT"), 27017 TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Mongo, "VERSION", strlen("VERSION"), PHP_MONGO_VERSION TSRMLS_CC);

  /* Mongo fields */
  zend_declare_property_bool(mongo_ce_Mongo, "connected", strlen("connected"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Mongo, "status", strlen("status"), ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Mongo, "server", strlen("server"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Mongo, "persistent", strlen("persistent"), ZEND_ACC_PROTECTED TSRMLS_CC);
}

/*
 * this deals with the new mongo connection format:
 * mongodb://username:password@host:port,host:port
 *
 * throws exception
 */
static int php_mongo_parse_server(zval *this_ptr TSRMLS_DC) {
  zval *hosts_z;
  char *hosts, *current;
  mongo_link *link;
  mongo_server *current_server;

  log0("parsing servers");

  hosts_z = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
  hosts = Z_STRLEN_P(hosts_z) ? Z_STRVAL_P(hosts_z) : 0;
  current = hosts;

  link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  // go with the default setup:
  // one connection to localhost:27017
  if (!hosts) {
    // set the top-level server set fields
    link->server_set = (mongo_server_set*)emalloc(sizeof(mongo_server_set));
    link->server_set->num = 1;
    link->server_set->ts = 0;
    link->server_set->server_ts = 0;

    // allocate one server
    link->server_set->server = (mongo_server*)emalloc(sizeof(mongo_server));
    memset(link->server_set->server, 0, sizeof(mongo_server));

    link->server_set->server->host = estrdup(MonGlo(default_host));
    link->server_set->server->port = MonGlo(default_port);
    spprintf(&link->server_set->server->label, 0, "%s:%d", MonGlo(default_host), MonGlo(default_port));
    link->server_set->master = link->server_set->server;

    return SUCCESS;
  }

  // check if it has the right prefix for a mongo connection url
  if (strstr(hosts, "mongodb://") == hosts) {
    char *at, *colon;

    // mongodb://user:pass@host:port,host:port
    //           ^
    current += 10;

    // mongodb://user:pass@host:port,host:port
    //                    ^
    at = strchr(current, '@');

    // mongodb://user:pass@host:port,host:port
    //               ^
    colon = strchr(current, ':');

    // check for username:password
    if (at && colon && at - colon > 0) {
      if (!link->username) {
        link->username = estrndup(current, colon-current);
      }
      if (!link->password) {
        link->password = estrndup(colon+1, at-(colon+1));
      }

      // move current
      // mongodb://user:pass@host:port,host:port
      //                     ^
      current = at+1;
    }
  }

  // now we're doing the same thing, regardless of prefix
  // host1[:27017][,host2[:27017]]+

  // allocate the server ptr
  link->server_set = (mongo_server_set*)emalloc(sizeof(mongo_server_set));
  link->server_set->ts = 0;
  link->server_set->server_ts = 0;

  // allocate the top-level server set fields
  link->server_set->num = 0;
  link->server_set->master = 0;

  // set server to 0 in case something goes wrong, then it won't be freed
  link->server_set->server = current_server = 0;

  // current is now pointing at the first server name

  // normal hostname
  while (*current) {
    mongo_server *server;
    char **current_ptr = &current;

    log1("current: %s", current);

    // method throws exception
    if (!(server = create_mongo_server(current_ptr, hosts, link TSRMLS_CC))) {
      zend_throw_exception_ex(mongo_ce_ConnectionException, 10 TSRMLS_CC,
                              "Couldn't parse %s (original: %s)", current, hosts);
      return FAILURE;
    }
    current = *current_ptr;

    link->server_set->num++;

    // initialize server list
    if (link->server_set->server == 0) {
      link->server_set->server = server;
      current_server = link->server_set->server;
    }
    // add a server
    else {
      current_server->next = server;
      current_server = current_server->next;
    }

    // localhost/dbname
    //          ^
    if (*current == '/') {
      break;
    }
    // localhost,
    // localhost
    //          ^
    if (*current == ',') {
      current++;
      while (*current == ' ') {
        current++;
      }
    }
  }

  // if this isn't the (invalid) form "host:port/"
  if (*current == '/' && *(current+1) != '\0') {
    current++;
    if (!link->db) {
      link->db = estrdup(current);
    }
  }

  // if we need to authenticate but weren't given a database, assume admin
  if (link->username && link->password) {
    mongo_server *c;

    if (!link->db) {
      link->db = estrdup("admin");
    }

    c = link->server_set->server;
    while (c) {
      c->db = estrdup(link->db);
      c->username = estrdup(link->username);
      c->password = estrdup(link->password);
      c = c->next;
    }
  }

  log1("done parsing", current);

  return SUCCESS;
}

mongo_server* create_mongo_server(char **current, char *hosts, mongo_link *link TSRMLS_DC) {
  char *host;
  int port;
  mongo_server *server;
  int domain_socket = 0;

  // localhost:1234
  // localhost,
  // localhost
  // /tmp/mongodb-port.sock
  // ^
  if (**current == '/') {
    domain_socket = 1;
  }

  if ((host = php_mongo_get_host(current, domain_socket)) == 0) {
    return 0;
  }

  // localhost:27017
  //          ^
  if (domain_socket) {
    port = 0;
    if (**current == ':') {
      (*current)++;
      while (**current >= '0' && **current <= '9') {
        (*current)++;
      }
    }
  }
  else if ((port = php_mongo_get_port(current)) < 0) {
    efree(host);
    return 0;
  }

  // create a struct for this server
  server = (mongo_server*)emalloc(sizeof(mongo_server));
  memset(server, 0, sizeof(mongo_server));

  server->host = host;
  server->port = port;
  spprintf(&server->label, 0, "%s:%d", host, port);

  return server;
}

/* {{{ Mongo->__construct
 */
PHP_METHOD(Mongo, __construct) {
  char *server = 0;
  int server_len = 0;
  zend_bool persist = 0, garbage = 0, connect = 1;
  zval *options = 0, *slave_okay = 0;
  mongo_link *link;
  mongo_server *current;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|szbb", &server, &server_len, &options, &persist, &garbage) == FAILURE) {
    return;
  }

  link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  slave_okay = zend_read_static_property(mongo_ce_Cursor, "slaveOkay", strlen("slaveOkay"), NOISY TSRMLS_CC);
  link->slave_okay = Z_BVAL_P(slave_okay);

  // new format
  if (options) {
    if (!IS_SCALAR_P(options)) {
      zval **timeout_z, **replica_z, **slave_okay_z, **username_z, **password_z,
        **db_z, **connect_z;

      if (zend_hash_find(HASH_P(options), "timeout", strlen("timeout")+1, (void**)&timeout_z) == SUCCESS) {
        link->timeout = Z_LVAL_PP(timeout_z);
      }
      if (zend_hash_find(HASH_P(options), "replicaSet", strlen("replicaSet")+1, (void**)&replica_z) == SUCCESS) {
        link->rs = Z_BVAL_PP(replica_z);
      }
      else {
        link->rs = 0;
      }

      if (zend_hash_find(HASH_P(options), "slaveOkay", strlen("slaveOkay")+1, (void**)&slave_okay_z) == SUCCESS) {
        link->slave_okay = Z_BVAL_PP(slave_okay_z);
      }
      if (zend_hash_find(HASH_P(options), "username", sizeof("username"), (void**)&username_z) == SUCCESS) {
        link->username = estrdup(Z_STRVAL_PP(username_z));
      }
      if (zend_hash_find(HASH_P(options), "password", sizeof("password"), (void**)&password_z) == SUCCESS) {
        link->password = estrdup(Z_STRVAL_PP(password_z));
      }
      if (zend_hash_find(HASH_P(options), "db", sizeof("db"), (void**)&db_z) == SUCCESS) {
        link->db = estrdup(Z_STRVAL_PP(db_z));
      }
      if (zend_hash_find(HASH_P(options), "connect", sizeof("connect"), (void**)&connect_z) == SUCCESS) {
        connect = Z_BVAL_PP(connect_z);
      }
    }
    else {
      // backwards compatibility
      connect = Z_BVAL_P(options);
      if (MonGlo(allow_persistent) && persist) {
        zend_update_property_string(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), "" TSRMLS_CC);
      }
    }
  }

  // If someone accidently does something like $hst instead of $host, we'll get
  // the empty string.
  if (server && strlen(server) == 0) {
    zend_throw_exception(mongo_ce_ConnectionException, "no server name given", 1 TSRMLS_CC);
  }
  zend_update_property_stringl(mongo_ce_Mongo, getThis(), "server", strlen("server"), server, server_len TSRMLS_CC);

  // parse the server name given
  if (php_mongo_parse_server(getThis() TSRMLS_CC) == FAILURE) {
    // exception thrown in parse_server
    return;
  }

  // initialize any connection pools needed (doesn't actually connect)
  current = link->server_set->server;
  while (current) {
    mongo_util_pool_init(current, (time_t)link->timeout TSRMLS_CC);
    current = current->next;
  }

  if (connect) {
    MONGO_METHOD(Mongo, connectUtil, return_value, getThis());
  }
}
/* }}} */


/* {{{ Mongo->connect
 */
PHP_METHOD(Mongo, connect) {
  MONGO_METHOD(Mongo, connectUtil, return_value, getThis());
}

/* {{{ Mongo->pairConnect
 */
PHP_METHOD(Mongo, pairConnect) {
  zend_error(E_WARNING, "Deprecated, use constructor instead");
}

/* {{{ Mongo->persistConnect
 */
PHP_METHOD(Mongo, persistConnect) {
  zend_error(E_WARNING, "Deprecated, use constructor instead");
}

/* {{{ Mongo->pairPersistConnect
 */
PHP_METHOD(Mongo, pairPersistConnect) {
  zend_error(E_WARNING, "Deprecated, use constructor instead");
}

PHP_METHOD(Mongo, connectUtil) {
  int connected = 0;
  mongo_link *link;
  mongo_server *current;
  char *msg = 0;
  zval *connected_z = 0;

  connected_z = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  if (Z_BVAL_P(connected_z)) {
    RETURN_TRUE;
  }

  link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  current = link->server_set->server;
  while (current) {
    zval *errmsg;
    MAKE_STD_ZVAL(errmsg);
    ZVAL_NULL(errmsg);

    connected |= (mongo_util_pool_get(current, errmsg TSRMLS_CC) == SUCCESS);

    if (!msg && Z_TYPE_P(errmsg) == IS_STRING) {
      msg = estrndup(Z_STRVAL_P(errmsg), Z_STRLEN_P(errmsg));
    }
    zval_ptr_dtor(&errmsg);
    current = current->next;
  }

  if (!connected) {
    zend_throw_exception(mongo_ce_ConnectionException, msg, 0 TSRMLS_CC);
  }
  else {
    mongo_util_rs_get_hosts(link TSRMLS_CC);
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected",
                              strlen("connected"), 1 TSRMLS_CC);
    ZVAL_BOOL(return_value, 1);
  }

  if (msg) {
    efree(msg);
  }
}

// get the next host from the server string
static char* php_mongo_get_host(char **ip, int domain_socket) {
  char *end = *ip, *retval;

  // pick whichever exists and is sooner: ':', ',', '/', or '\0'
  while (*end && *end != ',' && *end != ':' && (*end != '/' || domain_socket)) {
    end++;
  }

  // sanity check
  if (end - *ip > 1 && end - *ip < 256) {
    int len = end-*ip;

    // return a copy
    retval = estrndup(*ip, len);

    // move to the end of this section of string
    *(ip) = end;

    return retval;
  }
  else {
    // you get nothing
    return 0;
  }

  // otherwise, this is the last thing in the string
  retval = estrdup(*ip);

  // move to the end of this string
  *(ip) = *ip + strlen(*ip);

  return retval;
}

// get the next port from the server string
static int php_mongo_get_port(char **ip) {
  char *end;
  int retval;

  // there might not even be a port
  if (**ip != ':') {
    return 27017;
  }

  // if there is, move past the colon
  // localhost:27017
  //          ^
  (*ip)++;

  end = *ip;
  // make sure the port is actually a number
  while (*end >= '0' && *end <= '9') {
    end++;
  }

  if (end == *ip) {
    return -1;
  }

  // this just takes the first numeric characters
  retval = atoi(*ip);

  // move past the port
  *(ip) += (end - *ip);

  return retval;
}


/* {{{ Mongo->close()
 */
PHP_METHOD(Mongo, close) {
  mongo_link *link;

  PHP_MONGO_GET_LINK(getThis());

  mongo_util_link_disconnect(link);

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected",
                            strlen("connected"), 0 TSRMLS_CC);
  RETURN_TRUE;
}
/* }}} */

static char* stringify_server(mongo_server *server, char *str, int *pos, int *len) {
  // length: "[" + strlen(hostname) + ":" + strlen(port (maxint=12)) + "]"
  if (*len - *pos < (int)strlen(server->host)+15) {
    int new_len = *len + 256 + (2 * (strlen(server->host)+15));
    str = (char*)erealloc(str, new_len);
    *(len) = new_len;
  }

  // if this host is not connected, enclose in []s: [localhost:27017]
  if (!server->connected) {
    str[*pos] = '[';
    *(pos) = *pos + 1;
  }

  // copy host
  memcpy(str+*pos, server->label, strlen(server->label));
  *(pos) = *pos + strlen(server->label);

  // close []s
  if (!server->connected) {
    str[*pos] = ']';
    *(pos) = *pos + 1;
  }

  return str;
}


/* {{{ Mongo->__toString()
 */
PHP_METHOD(Mongo, __toString) {
  int tpos = 0, tlen = 256, *pos, *len;
  char *str;
  mongo_server *server;
  mongo_link *link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  // if we haven't connected yet, we should still be able to get the __toString
  // from the server field
  if (!link->server_set) {
    zval *server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
    RETURN_ZVAL(server, 1, 0);
  }

  pos = &tpos;
  len = &tlen;
  str = (char*)emalloc(*len);

  // stringify the master, if there is one
  if (link->server_set->master) {
    str = stringify_server(link->server_set->master, str, pos, len);
  }

  server = link->server_set->server;
  // stringify each server
  while (server) {
    if (server == link->server_set->master) {
      server = server->next;
      continue;
    }

    // if this is not the first one, add a comma
    if (*pos != 0) {
      str[*pos] = ',';
      *(pos) = *pos+1;
    }

    str = stringify_server(server, str, pos, len);
    server = server->next;
  }

  str[*pos] = '\0';

  RETURN_STRING(str, 0);
}
/* }}} */


/* {{{ Mongo->selectDB()
 */
PHP_METHOD(Mongo, selectDB) {
  zval temp, *name;
  char *db;
  int db_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &db, &db_len) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, db, 1);

  object_init_ex(return_value, mongo_ce_DB);
  MONGO_METHOD2(MongoDB, __construct, &temp, return_value, getThis(), name);

  zval_ptr_dtor(&name);
}
/* }}} */


/* {{{ Mongo::__get
 */
PHP_METHOD(Mongo, __get) {
  zval *name;
  char *str;
  int str_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, str, 1);

  // select this db
  MONGO_METHOD1(Mongo, selectDB, return_value, getThis(), name);

  zval_ptr_dtor(&name);
}
/* }}} */


/* {{{ Mongo::selectCollection()
 */
PHP_METHOD(Mongo, selectCollection) {
  char *db, *coll;
  int db_len, coll_len;
  zval *db_name, *coll_name, *temp_db;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &db, &db_len, &coll, &coll_len) == FAILURE) {
    return;
  }

  MAKE_STD_ZVAL(db_name);
  ZVAL_STRING(db_name, db, 1);

  MAKE_STD_ZVAL(temp_db);
  MONGO_METHOD1(Mongo, selectDB, temp_db, getThis(), db_name);
  zval_ptr_dtor(&db_name);
  PHP_MONGO_CHECK_EXCEPTION1(&temp_db);

  MAKE_STD_ZVAL(coll_name);
  ZVAL_STRING(coll_name, coll, 1);

  MONGO_METHOD1(MongoDB, selectCollection, return_value, temp_db, coll_name);

  zval_ptr_dtor(&coll_name);
  zval_ptr_dtor(&temp_db);
}
/* }}} */

PHP_METHOD(Mongo, getSlaveOkay) {
  mongo_link *link;
  PHP_MONGO_GET_LINK(getThis());
  RETURN_BOOL(link->slave_okay);
}

PHP_METHOD(Mongo, setSlaveOkay) {
  zend_bool slave_okay = 1;
  mongo_link *link;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &slave_okay) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_LINK(getThis());

  RETVAL_BOOL(link->slave_okay);
  link->slave_okay = slave_okay;
}


/* {{{ Mongo::dropDB()
 */
PHP_METHOD(Mongo, dropDB) {
  zval *db, *temp_db;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &db) == FAILURE) {
    RETURN_FALSE;
  }

  if (Z_TYPE_P(db) != IS_OBJECT ||
      Z_OBJCE_P(db) != mongo_ce_DB) {
    MAKE_STD_ZVAL(temp_db);

    // reusing db param from Mongo::drop call
    MONGO_METHOD_BASE(Mongo, selectDB)(1, temp_db, NULL, getThis(), 0 TSRMLS_CC);
    db = temp_db;
  }
  else {
    zval_add_ref(&db);
  }

  MONGO_METHOD(MongoDB, drop, return_value, db);
  zval_ptr_dtor(&db);
}
/* }}} */

/* {{{ Mongo->listDBs
 */
PHP_METHOD(Mongo, listDBs) {
  zval *admin, *data, *db;

  MAKE_STD_ZVAL(admin);
  ZVAL_STRING(admin, "admin", 1);

  MAKE_STD_ZVAL(db);

  MONGO_METHOD1(Mongo, selectDB, db, getThis(), admin);

  zval_ptr_dtor(&admin);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "listDatabases", 1);

  MONGO_CMD(return_value, db);

  zval_ptr_dtor(&data);
  zval_ptr_dtor(&db);
}
/* }}} */

PHP_METHOD(Mongo, getHosts) {
  return;
}

PHP_METHOD(Mongo, getSlave) {
  mongo_link *link;

  PHP_MONGO_GET_LINK(getThis());

  if (link->rs && link->slave) {
    RETURN_STRING(link->slave->label, 1);
  }
}

PHP_METHOD(Mongo, switchSlave) {
  mongo_link *link;
  char *errmsg = 0;

  PHP_MONGO_GET_LINK(getThis());

  if (!link->rs) {
    zend_throw_exception(mongo_ce_Exception,
                         "Reading from slaves won't work without using the replicaSet option on connect",
                         15 TSRMLS_CC);
    return;
  }

  if (get_heartbeats(getThis(), &errmsg TSRMLS_CC) == FAILURE ||
      set_a_slave(link, &errmsg) == FAILURE) {
    if (!EG(exception)) {
      if (errmsg) {
        zend_throw_exception(mongo_ce_Exception, errmsg, 16 TSRMLS_CC);
        efree(errmsg);
      }
      else {
        zend_throw_exception(mongo_ce_Exception, "No server found for reads", 16 TSRMLS_CC);
      }
    }
    return;
  }

  MONGO_METHOD(Mongo, getSlave, return_value, getThis());
}

static void run_err(int err_type, zval *return_value, zval *this_ptr TSRMLS_DC) {
  zval *db_name, *db;
  MAKE_STD_ZVAL(db_name);
  ZVAL_STRING(db_name, "admin", 1);

  MAKE_STD_ZVAL(db);
  MONGO_METHOD1(Mongo, selectDB, db, getThis(), db_name);
  zval_ptr_dtor(&db_name);

  switch (err_type) {
  case LAST_ERROR:
    MONGO_METHOD(MongoDB, lastError, return_value, db);
    break;
  case PREV_ERROR:
    MONGO_METHOD(MongoDB, prevError, return_value, db);
    break;
  case RESET_ERROR:
    MONGO_METHOD(MongoDB, resetError, return_value, db);
    break;
  case FORCE_ERROR:
    MONGO_METHOD(MongoDB, forceError, return_value, db);
    break;
  }

  zval_ptr_dtor(&db);
}


/* {{{ Mongo->lastError()
 */
PHP_METHOD(Mongo, lastError) {
  run_err(LAST_ERROR, return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ Mongo->prevError()
 */
PHP_METHOD(Mongo, prevError) {
  run_err(PREV_ERROR, return_value, getThis() TSRMLS_CC);
}
/* }}} */


/* {{{ Mongo->resetError()
 */
PHP_METHOD(Mongo, resetError) {
  run_err(RESET_ERROR, return_value, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ Mongo->forceError()
 */
PHP_METHOD(Mongo, forceError) {
  run_err(FORCE_ERROR, return_value, getThis() TSRMLS_CC);
}
/* }}} */

/*
 * This method reads the message header for a database response
 * It returns failure or success and throws an exception on failure.
 */
static int get_header(int sock, mongo_cursor *cursor TSRMLS_DC) {
  // set a timeout
  if (cursor->timeout && cursor->timeout > 0) {
    struct timeval timeout;

    timeout.tv_sec = cursor->timeout / 1000 ;
    timeout.tv_usec = (cursor->timeout % 1000) * 1000;

    while (1) {
      int status;
      fd_set readfds, exceptfds;

      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);
      FD_ZERO(&exceptfds);
      FD_SET(sock, &exceptfds);

      status = select(sock+1, &readfds, NULL, &exceptfds, &timeout);

      if (status == -1) {
        zend_throw_exception(mongo_ce_CursorException, strerror(errno), 13
                             TSRMLS_CC);
        return FAILURE;
      }

      if (FD_ISSET(sock, &exceptfds)) {
        zend_throw_exception(mongo_ce_CursorException,
                             "Exceptional condition on socket", 17 TSRMLS_CC);
        return FAILURE;
      }

      if (status == 0 && !FD_ISSET(sock, &readfds)) {
        zend_throw_exception_ex(mongo_ce_CursorTOException, 0 TSRMLS_CC,
                                "cursor timed out (timeout: %d, time left: %d:%d, status: %d)",
                                cursor->timeout, timeout.tv_sec, timeout.tv_usec,
                                status);
        return FAILURE;
      }

      // if our descriptor is ready break out
      if (FD_ISSET(sock, &readfds)) {
        break;
      }
    }
  }

  if (recv(sock, (char*)&cursor->recv.length, INT_32, FLAGS) < INT_32) {
    zend_throw_exception(mongo_ce_CursorException, "couldn't get response header", 4 TSRMLS_CC);
    return FAILURE;
  }

  // switch the byte order, if necessary
  cursor->recv.length = MONGO_32(cursor->recv.length);

  // make sure we're not getting crazy data
  if (cursor->recv.length == 0) {
    zend_throw_exception(mongo_ce_CursorException, "no db response", 5 TSRMLS_CC);
    return FAILURE;
  }
  else if (cursor->recv.length < REPLY_HEADER_SIZE) {
    zend_throw_exception_ex(mongo_ce_CursorException, 6 TSRMLS_CC,
                            "bad response length: %d, did the db assert?",
                            cursor->recv.length);
    return FAILURE;
  }

  if (recv(sock, (char*)&cursor->recv.request_id, INT_32, FLAGS) < INT_32 ||
      recv(sock, (char*)&cursor->recv.response_to, INT_32, FLAGS) < INT_32 ||
      recv(sock, (char*)&cursor->recv.op, INT_32, FLAGS) < INT_32) {
    zend_throw_exception(mongo_ce_CursorException, "incomplete header", 7 TSRMLS_CC);
    return FAILURE;
  }

  cursor->recv.request_id = MONGO_32(cursor->recv.request_id);
  cursor->recv.response_to = MONGO_32(cursor->recv.response_to);
  cursor->recv.op = MONGO_32(cursor->recv.op);

  if (cursor->recv.response_to > MonGlo(response_num)) {
    MonGlo(response_num) = cursor->recv.response_to;
  }

  return SUCCESS;
}

static int get_cursor_body(int sock, mongo_cursor *cursor TSRMLS_DC) {
  int num_returned = 0;

  if (recv(sock, (char*)&cursor->flag, INT_32, FLAGS) < INT_32 ||
      recv(sock, (char*)&cursor->cursor_id, INT_64, FLAGS) < INT_64 ||
      recv(sock, (char*)&cursor->start, INT_32, FLAGS) < INT_32 ||
      recv(sock, (char*)&num_returned, INT_32, FLAGS) < INT_32) {
    zend_throw_exception(mongo_ce_CursorException, "incomplete response", 8 TSRMLS_CC);
    return FAILURE;
  }

  cursor->cursor_id = MONGO_64(cursor->cursor_id);
  cursor->flag = MONGO_32(cursor->flag);
  cursor->start = MONGO_32(cursor->start);
  num_returned = MONGO_32(num_returned);

  /* cursor->num is the total of the elements we've retrieved
   * (elements already iterated through + elements in db response
   * but not yet iterated through)
   */
  cursor->num += num_returned;

  // create buf
  cursor->recv.length -= REPLY_HEADER_LEN;

  if (cursor->buf.start) {
    efree(cursor->buf.start);
  }

  cursor->buf.start = (char*)emalloc(cursor->recv.length);
  cursor->buf.end = cursor->buf.start + cursor->recv.length;
  cursor->buf.pos = cursor->buf.start;

  // finish populating cursor
  return mongo_hear(sock, cursor->buf.pos, cursor->recv.length TSRMLS_CC);
}

/*
 * throws exception on FAILURE
 */
int php_mongo_get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC) {
  int sock;

  log0("hearing something");

  LOCK;

  // this cursor has already been processed
  if (cursor->send.request_id < MonGlo(response_num)) {
    cursor_node *response = 0;
    list_entry *le;

    if (zend_hash_find(&EG(persistent_list), "response_list", strlen("response_list") + 1, (void**)&le) == SUCCESS) {
      response = le->ptr;
    }

    while (response) {
      if (response->cursor->recv.response_to == cursor->send.request_id) {
        make_unpersistent_cursor(response->cursor, cursor);
        UNLOCK;
        php_mongo_free_cursor_node(response, le);
        return SUCCESS;
      }
      response = response->next;
    }

    // if we didn't find it, it might have been send out of order so keep going
  }

  sock = cursor->server->socket;

  if (get_header(sock, cursor TSRMLS_CC) == FAILURE) {
    UNLOCK;
    return FAILURE;
  }

  // check that this is actually the response we want
  while (cursor->send.request_id != cursor->recv.response_to) {
    log2("request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);

    // if it's not...

    // if we're getting the response to an earlier request, put the response on
    // the queue
    if (cursor->send.request_id > cursor->recv.response_to) {
      if (FAILURE != get_cursor_body(sock, cursor TSRMLS_CC)) {
        mongo_cursor *pcursor = make_persistent_cursor(cursor);
        // add to list
        UNLOCK;
        php_mongo_create_le(pcursor, "response_list" TSRMLS_CC);
        LOCK;
      }
      else {
        // else if we've failed, just don't add to queue
        // if we can reconnect, continue
        if (mongo_util_pool_failed(cursor->server, 0 TSRMLS_CC) == FAILURE) {
          zend_throw_exception(mongo_ce_CursorException, "lost db connection", 9 TSRMLS_CC);
          UNLOCK;
          return FAILURE;
        }
        mongo_util_rs_get_hosts(cursor->link TSRMLS_CC);
        if (!cursor->server->connected) {
          zend_throw_exception(mongo_ce_CursorException, "lost db connection (2)", 9 TSRMLS_CC);
          UNLOCK;
          return FAILURE;
        }
        sock = cursor->server->socket;
      }
    }
    // otherwise, check if the response is on the queue
    else {
      cursor_node *response = 0;
      list_entry *le;

      if (zend_hash_find(&EG(persistent_list), "response_list", strlen("response_list") + 1, (void**)&le) == SUCCESS) {
        response = le->ptr;
      }

      while (response) {
        // if it is, then pull it off & use it
        if (response->cursor->send.request_id == cursor->recv.response_to) {
          memcpy(cursor, response->cursor, sizeof(mongo_cursor));
          UNLOCK;
          php_mongo_free_cursor_node(response, le);
          return SUCCESS;
        }
        response = response->next;
      }

      if (!response) {
        UNLOCK;
        zend_throw_exception(mongo_ce_CursorException, "couldn't find a response", 9 TSRMLS_CC);
        return FAILURE;
      }
    }

    // get the next db response
    if (get_header(sock, cursor TSRMLS_CC) == FAILURE) {
      UNLOCK;
      return FAILURE;
    }
  }

  if (FAILURE == get_cursor_body(sock, cursor TSRMLS_CC)) {
    UNLOCK;
#ifdef WIN32
    zend_throw_exception_ex(mongo_ce_CursorException, 12 TSRMLS_CC, "WSA error getting database response: %d", WSAGetLastError());
#else
    zend_throw_exception_ex(mongo_ce_CursorException, 12 TSRMLS_CC, "error getting database response: %d", strerror(errno));
#endif
    return FAILURE;
  }

  UNLOCK;

  /* if no catastrophic error has happened yet, we're fine, set errmsg to null */
  ZVAL_NULL(errmsg);

  return SUCCESS;
}

static mongo_cursor* make_persistent_cursor(mongo_cursor *cursor) {
  mongo_cursor *pcursor;
  int len;

  pcursor = (mongo_cursor*)pemalloc(sizeof(mongo_cursor), 1);
  // copying the whole cursor is easier, but we'll only need certain fields
  memcpy(pcursor, cursor, sizeof(mongo_cursor));

  pcursor->recv.length = cursor->recv.length;
  pcursor->recv.request_id = cursor->recv.request_id;
  pcursor->recv.response_to = cursor->recv.response_to;
  pcursor->recv.op = cursor->recv.op;

  len = cursor->buf.end - cursor->buf.start;
  pcursor->buf.start = (char*)pemalloc(len, 1);
  memcpy(pcursor->buf.start, cursor, len);
  pcursor->buf.pos = pcursor->buf.start;
  pcursor->buf.end = pcursor->buf.start+len;

  return pcursor;
}

/*
 * Copy response fields from a persistent cursor to a normal cursor and then
 * free the persistent cursor.
 */
static void make_unpersistent_cursor(mongo_cursor *pcursor, mongo_cursor *cursor) {
  int len;

  // header
  cursor->recv.length = pcursor->recv.length;
  cursor->recv.request_id = pcursor->recv.request_id;
  cursor->recv.response_to = pcursor->recv.response_to;
  cursor->recv.op = pcursor->recv.op;

  // field populated by the response
  cursor->flag = pcursor->flag;
  cursor->cursor_id = pcursor->cursor_id;
  cursor->start = pcursor->start;
  cursor->num = pcursor->num;

  // the actual response
  len = pcursor->buf.end - pcursor->buf.start;
  cursor->buf.start = (char*)emalloc(len);
  memcpy(cursor->buf.start, pcursor, len);
  cursor->buf.pos = cursor->buf.start;
  cursor->buf.end = cursor->buf.start+len;

  // free persistent cursor
  pefree(pcursor->buf.start, 1);
  pefree(pcursor, 1);
}

/*
 * Low-level send function.
 *
 * Goes through the buffer sending 4K byte batches.
 * On failure, sets errmsg to errno string.
 * On success, returns number of bytes sent.
 * Does not attempt to reconnect nor throw any exceptions.
 *
 * On failure, the calling function is responsible for disconnecting
 */
int _mongo_say(int sock, buffer *buf, zval *errmsg TSRMLS_DC) {
  int sent = 0, total = 0, status = 1;

  log0("saying something");

  total = buf->pos - buf->start;

  while (sent < total && status > 0) {
    int len = 4096 < (total - sent) ? 4096 : total - sent;

    status = send(sock, (const char*)buf->start + sent, len, FLAGS);

    if (status == FAILURE) {
      ZVAL_STRING(errmsg, strerror(errno), 1);
      return FAILURE;
    }
    sent += status;
  }

  return sent;
}

int mongo_say(mongo_server *server, buffer *buf, zval *errmsg TSRMLS_CC) {
  if(!server->connected &&
     mongo_util_pool_get(server, errmsg TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  if (_mongo_say(server->socket, buf, errmsg TSRMLS_CC) == FAILURE) {
    // try to reconnect, but we can't retry the send regardless
    mongo_util_pool_failed(server, 0 TSRMLS_CC);
    return FAILURE;
  }
  return SUCCESS;
}

int mongo_hear(int sock, void *dest, int total_len TSRMLS_DC) {
  int num = 1, received = 0;

  // this can return FAILED if there is just no more data from db
  while(received < total_len && num > 0) {
    int len = 4096 < (total_len - received) ? 4096 : total_len - received;

    // windows gives a WSAEFAULT if you try to get more bytes
    num = recv(sock, (char*)dest, len, FLAGS);

    if (num < 0) {
      return FAILURE;
    }

    dest = (char*)dest + num;
    received += num;
  }
  return received;
}

