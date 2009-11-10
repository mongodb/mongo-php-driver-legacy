// mongo.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netdb.h>
#endif

#include <php.h>
#include <zend_exceptions.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#ifdef WIN32
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#endif

#include "php_mongo.h"
#include "db.h"
#include "cursor.h"
#include "mongo_types.h"
#include "bson.h"

extern zend_class_entry *mongo_ce_DB, 
  *mongo_ce_Cursor,
  *mongo_ce_Id,
  *mongo_ce_Date,
  *mongo_ce_Regex,
  *mongo_ce_Code,
  *mongo_ce_BinData,
  *mongo_ce_Timestamp;

static void mongo_link_dtor(mongo_link*);
static void connect_already(INTERNAL_FUNCTION_PARAMETERS, zval*);
static int get_master(mongo_link* TSRMLS_DC);
static int check_connection(mongo_link*, zval* TSRMLS_DC);
static int mongo_connect_nonb(int, char*, int, zval*);
static int mongo_do_socket_connect(mongo_link*, zval* TSRMLS_DC);
static int mongo_get_sockaddr(struct sockaddr_in*, char*, int, zval*);
static char* getHost(char*, int);
static int getPort(char*);
static int get_host_and_port(char*, mongo_link*, zval* TSRMLS_DC);
static void mongo_init_MongoExceptions(TSRMLS_D);
static void run_err(int, zval*, zval* TSRMLS_DC);

inline void set_disconnected(mongo_link *link);

zend_object_handlers mongo_default_handlers;

/** Classes */
zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_CursorException,
  *mongo_ce_ConnectionException,
  *mongo_ce_GridFSException,
  *mongo_ce_Exception;

/** Resources */
int le_connection, le_pconnection;

ZEND_DECLARE_MODULE_GLOBALS(mongo)

#if ZEND_MODULE_API_NO >= 20060613
// 5.2+ globals
static PHP_GINIT_FUNCTION(mongo);
#else
// 5.1- globals
static void mongo_init_globals(zend_mongo_globals* g TSRMLS_DC);
#endif /* ZEND_MODULE_API_NO >= 20060613 */

function_entry mongo_functions[] = {
  PHP_FE(bson_encode, NULL)
  PHP_FE(bson_decode, NULL)
  { NULL, NULL, NULL }
};

static function_entry mongo_methods[] = {
  PHP_ME(Mongo, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, connect, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, pairConnect, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, persistConnect, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, pairPersistConnect, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, connectUtil, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Mongo, __toString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, selectDB, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, selectCollection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, dropDB, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, lastError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, prevError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, resetError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, forceError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
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
PHP_INI_BEGIN()
STD_PHP_INI_BOOLEAN("mongo.auto_reconnect", "0", PHP_INI_SYSTEM, OnUpdateLong, auto_reconnect, zend_mongo_globals, mongo_globals)
STD_PHP_INI_BOOLEAN("mongo.allow_persistent", "1", PHP_INI_SYSTEM, OnUpdateLong, allow_persistent, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY_EX("mongo.max_persistent", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_persistent, zend_mongo_globals, mongo_globals, display_link_numbers)
STD_PHP_INI_ENTRY_EX("mongo.max_connections", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_links, zend_mongo_globals, mongo_globals, display_link_numbers)
STD_PHP_INI_ENTRY("mongo.default_host", "localhost", PHP_INI_ALL, OnUpdateString, default_host, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.default_port", "27017", PHP_INI_ALL, OnUpdateLong, default_port, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.chunk_size", "262144", PHP_INI_ALL, OnUpdateLong, default_port, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.cmd", "$", PHP_INI_ALL, OnUpdateString, cmd_char, zend_mongo_globals, mongo_globals)
PHP_INI_END()
/* }}} */



static void mongo_link_dtor(mongo_link *link) {
  if (link) {
    if (link->paired) {
#ifdef WIN32
      closesocket(link->server.paired.lsocket);
      closesocket(link->server.paired.rsocket);
#else
      close(link->server.paired.lsocket);
      close(link->server.paired.rsocket);
#endif

      if (link->server.paired.left) {
        pefree(link->server.paired.left, link->persist);
      }
      if (link->server.paired.right) {
        pefree(link->server.paired.right, link->persist);
      }
    }
    else {
      if (link->server.single.socket > -1) {
      // close the connection
#ifdef WIN32
        closesocket(link->server.single.socket);
#else
        close(link->server.single.socket);
#endif
        link->server.single.socket = -1;
      }

      // free strings
      if (link->server.single.host) {
        pefree(link->server.single.host, link->persist);
        link->server.single.host = 0;
      }
    }

    if (link->username) {
      pefree(link->username, link->persist);
      link->username = 0;
    }
    if (link->password) {
      pefree(link->password, link->persist);
      link->password = 0;
    }

    // free connection
    pefree(link, link->persist);
  }
}

static void php_connection_dtor( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo_link_dtor((mongo_link*)rsrc->ptr);
  // if it's a persistent connection, decrement the
  // number of open persistent links
  if (rsrc->type == le_pconnection) {
    MonGlo(num_persistent)--;
  }
  // decrement the total number of links
  MonGlo(num_links)--;
  rsrc->ptr = 0;
}



/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mongo) {

#if ZEND_MODULE_API_NO < 20060613
  ZEND_INIT_MODULE_GLOBALS(mongo, mongo_init_globals, NULL);
#endif /* ZEND_MODULE_API_NO < 20060613 */

  REGISTER_INI_ENTRIES();

  le_connection = zend_register_list_destructors_ex(php_connection_dtor, NULL, PHP_CONNECTION_RES_NAME, module_number);
  le_pconnection = zend_register_list_destructors_ex(NULL, php_connection_dtor, PHP_CONNECTION_RES_NAME, module_number);

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

  // make mongo objects uncloneable
  memcpy(&mongo_default_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mongo_default_handlers.clone_obj = NULL;

  // start random number generator
  srand(time(0));
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
  struct hostent *lh;
  char *arKey;
  int nKeyLength;
  register ulong hash;

  mongo_globals->num_persistent = 0;
  mongo_globals->num_links = 0;
  mongo_globals->auto_reconnect = 0;
  mongo_globals->default_host = "localhost";
  mongo_globals->default_port = 27017;
  mongo_globals->request_id = 3;
  mongo_globals->chunk_size = DEFAULT_CHUNK_SIZE;
  mongo_globals->cmd_char = 0;
  mongo_globals->inc = 0;

  lh = gethostbyname("localhost");
  arKey = lh ? lh->h_name : "borkdebork";
  nKeyLength = strlen(arKey);
  hash = 5381;

  /* from zend_hash.h */
  /* variant with the hash unrolled eight times */
  for (; nKeyLength >= 8; nKeyLength -= 8) {
    hash = ((hash << 5) + hash) + *arKey++;
    hash = ((hash << 5) + hash) + *arKey++;
    hash = ((hash << 5) + hash) + *arKey++;
    hash = ((hash << 5) + hash) + *arKey++;
    hash = ((hash << 5) + hash) + *arKey++;
    hash = ((hash << 5) + hash) + *arKey++;
    hash = ((hash << 5) + hash) + *arKey++;
    hash = ((hash << 5) + hash) + *arKey++;
  }
  switch (nKeyLength) {
  case 7: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
  case 6: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
  case 5: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
  case 4: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
  case 3: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
  case 2: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
  case 1: hash = ((hash << 5) + hash) + *arKey++; break;
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

  return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(mongo) {
  MonGlo(num_links) = MonGlo(num_persistent);

  return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mongo) {
  char buf[32];

  php_info_print_table_start();

  php_info_print_table_header(2, "MongoDB Support", "enabled");
  snprintf(buf, sizeof(buf), "%ld", MonGlo(num_persistent));
  php_info_print_table_row(2, "Active Persistent Connections", buf);
  snprintf(buf, sizeof(buf), "%ld", MonGlo(num_links));
  php_info_print_table_row(2, "Active Connections", buf);

  php_info_print_table_end();

  DISPLAY_INI_ENTRIES();
}
/* }}} */

void mongo_init_MongoExceptions(TSRMLS_D) {
  zend_class_entry e, ce, conn, e2;

  INIT_CLASS_ENTRY(e, "MongoException", NULL);

#if ZEND_MODULE_API_NO >= 20060613
  mongo_ce_Exception = zend_register_internal_class_ex(&e, (zend_class_entry*)zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
#else
  mongo_ce_Exception = zend_register_internal_class_ex(&e, (zend_class_entry*)zend_exception_get_default(), NULL TSRMLS_CC);
#endif /* ZEND_MODULE_API_NO >= 20060613 */

  INIT_CLASS_ENTRY(ce, "MongoCursorException", NULL);
  mongo_ce_CursorException = zend_register_internal_class_ex(&ce, mongo_ce_Exception, NULL TSRMLS_CC);

  INIT_CLASS_ENTRY(conn, "MongoConnectionException", NULL);
  mongo_ce_ConnectionException = zend_register_internal_class_ex(&conn, mongo_ce_Exception, NULL TSRMLS_CC);

  INIT_CLASS_ENTRY(e2, "MongoGridFSException", NULL);
  mongo_ce_GridFSException = zend_register_internal_class_ex(&e2, mongo_ce_Exception, NULL TSRMLS_CC);
}

void mongo_init_Mongo(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "Mongo", mongo_methods);
  mongo_ce_Mongo = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_string(mongo_ce_Mongo, "DEFAULT_HOST", strlen("DEFAULT_HOST"), MonGlo(default_host) TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Mongo, "DEFAULT_PORT", strlen("DEFAULT_PORT"), MonGlo(default_port) TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Mongo, "VERSION", strlen("VERSION"), "1.0.0+" TSRMLS_CC);

  zend_declare_property_bool(mongo_ce_Mongo, "connected", strlen("connected"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);

  zend_declare_property_string(mongo_ce_Mongo, "server", strlen("server"), "localhost:27017", ZEND_ACC_PROTECTED TSRMLS_CC);

  zend_declare_property_string(mongo_ce_Mongo, "username", strlen("username"), "", ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_string(mongo_ce_Mongo, "password", strlen("password"), "", ZEND_ACC_PROTECTED TSRMLS_CC);

  zend_declare_property_bool(mongo_ce_Mongo, "paired", strlen("paired"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_bool(mongo_ce_Mongo, "persistent", strlen("persistent"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);

  zend_declare_property_null(mongo_ce_Mongo, "connection", strlen("connection"), ZEND_ACC_PUBLIC TSRMLS_CC);
}

/* {{{ Mongo->__construct
 */
PHP_METHOD(Mongo, __construct) {
  char *server = 0;
  int server_len = 0;
  zend_bool connect = 1, paired = 0, persist = 0;
  zval *zserver;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sbbb", &server, &server_len, &connect, &persist, &paired) == FAILURE) {
    return;
  }

  if (!server) {
    zserver = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
    server = Z_STRVAL_P(zserver);
    server_len = Z_STRLEN_P(zserver);
  }

  zend_update_property_stringl(mongo_ce_Mongo, getThis(), "server", strlen("server"), server, server_len TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), paired TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), persist TSRMLS_CC);

  if (connect) {
    MONGO_METHOD(Mongo, connect, return_value, getThis(), 0);
  }
  else {
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 0 TSRMLS_CC);
  }
}
/* }}} */


/* {{{ Mongo->connect
 */
PHP_METHOD(Mongo, connect) {
  MONGO_METHOD_BASE(Mongo, connectUtil)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* {{{ Mongo->pairConnect
 */
PHP_METHOD(Mongo, pairConnect) {
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), 1 TSRMLS_CC);
  MONGO_METHOD_BASE(Mongo, connectUtil)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* {{{ Mongo->persistConnect
 */
PHP_METHOD(Mongo, persistConnect) {
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), 1 TSRMLS_CC);

  MONGO_METHOD_BASE(Mongo, connectUtil)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* {{{ Mongo->pairPersistConnect
 */
PHP_METHOD(Mongo, pairPersistConnect) {
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), 1 TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), 1 TSRMLS_CC);

  MONGO_METHOD_BASE(Mongo, connectUtil)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}


PHP_METHOD(Mongo, connectUtil) {
  zval *connected, *server, *errmsg;
  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  // if we're already connected, disconnect
  connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  if (Z_BVAL_P(connected)) {
    // Mongo->close()
    MONGO_METHOD(Mongo, close, return_value, getThis(), 0);

    // Mongo->connected = false
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  }
  connect_already(INTERNAL_FUNCTION_PARAM_PASSTHRU, errmsg);

  connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  // if connecting failed, throw an exception
  if (!Z_BVAL_P(connected)) {
    char *full_error;
    server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);

    spprintf(&full_error, 0, "%s: %s", Z_STRVAL_P(server), Z_STRVAL_P(errmsg));
    zend_throw_exception(mongo_ce_ConnectionException, full_error, 0 TSRMLS_CC);

    zval_ptr_dtor(&errmsg);
    efree(full_error);
    return;
  }

  zval_ptr_dtor(&errmsg);
  // set the Mongo->connected property
  Z_LVAL_P(connected) = 1;
}


static void connect_already(INTERNAL_FUNCTION_PARAMETERS, zval *errmsg) {
  zval *username = 0, *password = 0, *server, *pair, *persist;
  mongo_link *link;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &username, &password) == FAILURE) {
    return;
  }

  server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
  pair = zend_read_property(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), NOISY TSRMLS_CC);
  persist = zend_read_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), NOISY TSRMLS_CC);

  /* 
   * check connection limits 
   */

  /* make sure that there aren't too many links already */
  if (MonGlo(max_links) > -1 &&
      MonGlo(max_links) <= MonGlo(num_links)) {
    char *errstr;
    spprintf(&errstr, 0, "more links than your body has room for: %ld of %ld", MonGlo(num_links), MonGlo(max_links));
    ZVAL_STRING(errmsg, errstr, 0);
    RETURN_FALSE;
  }
  /* if persistent links aren't allowed, just create a normal link */
  if (!MonGlo(allow_persistent)) {
    ZVAL_BOOL(persist, 0);
  }
  /* make sure that there aren't too many persistent links already */
  if (Z_BVAL_P(persist) &&
      MonGlo(max_persistent) > -1 &&
      MonGlo(max_persistent) <= MonGlo(num_persistent)) {
    char *errstr;
    spprintf(&errstr, 0, "more persistent links than your body has room for: %ld of %ld", MonGlo(num_persistent), MonGlo(max_persistent));
    ZVAL_STRING(errmsg, errstr, 0);
    RETURN_FALSE;
  }

  /* 
   * end of connection limit check
   */

  if (Z_BVAL_P(persist)) {
    zend_rsrc_list_entry *le;
    char *key;
    int key_len;

    key_len = spprintf(&key, 0, "%s_%s_%s", Z_STRVAL_P(server), 
		       username ? Z_STRVAL_P(username) : "", 
		       password ? Z_STRVAL_P(password) : "");

    // if a connection is found, return it
    if (zend_hash_find(&EG(persistent_list), key, key_len+1, (void**)&le) == SUCCESS) {
      zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 1 TSRMLS_CC);

      link = (mongo_link*)le->ptr;
      ZEND_REGISTER_RESOURCE(return_value, link, le_pconnection);
      zend_update_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), return_value TSRMLS_CC);
      efree(key);
      return;
    }
    efree(key);
  }

  if (Z_STRLEN_P(server) == 0) {
    ZVAL_STRING(errmsg, "no server name given", 1);
    return;
  }


  link = (mongo_link*)pemalloc(sizeof(mongo_link), Z_BVAL_P(persist));

  // zero pointers so it doesn't segfault on cleanup if
  // connection fails
  link->username = 0;
  link->password = 0;
  link->paired = Z_BVAL_P(pair);
  link->persist = Z_BVAL_P(persist);

  if (link->paired) {
    link->server.paired.left = 0;
    link->server.paired.right = 0;
  }
  else {
    link->server.single.host = 0;
  }

  if (get_host_and_port(Z_STRVAL_P(server), link, errmsg TSRMLS_CC) == FAILURE ||
      mongo_do_socket_connect(link, errmsg TSRMLS_CC) == FAILURE) {
    mongo_link_dtor(link);
    return;
  }

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 1 TSRMLS_CC);

  // store a reference in the persistence list
  if (Z_BVAL_P(persist)) {
    zend_rsrc_list_entry new_le;
    char *key;
    int key_len;

    // save username and password for reconnection
    if (username && Z_STRLEN_P(username) > 0) {
      link->username = (char*)pestrdup(Z_STRVAL_P(username), link->persist);
    }
    if (password && Z_STRLEN_P(password) > 0) {
      link->password = (char*)pestrdup(Z_STRVAL_P(password), link->persist);
    }

    key_len = spprintf(&key, 0, "%s_%s_%s", Z_STRVAL_P(server), 
		       username ? Z_STRVAL_P(username) : "", 
		       password ? Z_STRVAL_P(password) : "");

    Z_TYPE(new_le) = le_pconnection;
    new_le.ptr = link;

    if (zend_hash_update(&EG(persistent_list), key, key_len+1, (void*)&new_le, sizeof(zend_rsrc_list_entry), NULL)==FAILURE) {
      php_connection_dtor(&new_le TSRMLS_CC);
      efree(key);
      RETURN_FALSE;
    }
    efree(key);

    ZEND_REGISTER_RESOURCE(return_value, link, le_pconnection);
    MonGlo(num_persistent)++;
  }
  // otherwise, just return the connection
  else {
    ZEND_REGISTER_RESOURCE(return_value, link, le_connection);
  }
  zend_update_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), return_value TSRMLS_CC);

  MonGlo(num_links)++;
}

static char* getHost(char *ip, int persist) {
  char *colon = strchr(ip, ':');

  if (colon) {
    if (colon - ip > 1 && 
	colon -ip < 256) {
      int len = colon-ip;
      return persist ? 
	zend_strndup(ip, len) :
	estrndup(ip, len);
    }
    else {
      return 0;
    }
  }
  return pestrdup(ip, persist);
}

static int getPort(char *ip) {
  char *colon = strchr(ip, ':');

  if (colon && colon[1]) {
    int i;

    colon++;

    // make sure the port is actually a number
    for (i = 0; i < (ip+strlen(ip)) - colon; i++) {
      if (colon[i] < '0' ||
	  colon[i] > '9') {
	return 0;
      }
    }

    return atoi(colon);
  }
  return 27017;
}

static int get_host_and_port(char *server, mongo_link *link, zval *errmsg TSRMLS_DC) {
  char *host, *comma;
  int port;

  // extract host:port
  comma = strchr(server, ',');

  // two-part ip
  if (comma) {
    char *ip1;

    ip1 = estrndup(server, comma-server);

    if ((host = getHost(ip1, link->persist)) == 0 ||
	(port = getPort(ip1)) == 0) {
      char *errstr;
      spprintf(&errstr, 0, "failed to get left host [%s] or port [%d] from %s", 
               host ? host : "", port, server);
      ZVAL_STRING(errmsg, errstr, 0);

      if (host) {
	efree(host);
      }
      efree(ip1);

      return FAILURE;
    }

    efree(ip1);
  }
  else {
    if ((host = getHost(server, link->persist)) == 0 ||
	(port = getPort(server)) == 0) {
      char *errstr;
      spprintf(&errstr, 0, "failed to get host [%s] or port [%d] from %s", 
               host ? host : "", port, server);
      ZVAL_STRING(errmsg, errstr, 0);

      if (host) {
	efree(host);
      }
      return FAILURE;
    }
  }


  if (link->paired) {

    link->server.paired.left = host;
    link->server.paired.lport = port;


    // we get a string: host1:123,host2:456
    comma = strchr(server, ',');
    if (!comma) {
      ZVAL_STRING(errmsg, "paired connection, but no right host given", 1);

      // make sure the right hostname won't be 
      // cleaned up, as it wasn't set
      link->server.paired.right = 0;
      return FAILURE;
    }

    // get to host2:456
    comma++;

    if ((host = getHost(comma, link->persist)) == 0 ||
	(port = getPort(comma)) == 0) {
      char *errstr;
      spprintf(&errstr, 0, "failed to get right host [%s] or port [%d] from %s", 
               host ? host : "", port, comma);
      ZVAL_STRING(errmsg, errstr, 0);

      if (host) {
	efree(host);
      }
      return FAILURE;
    }

    link->server.paired.right = host;
    link->server.paired.rport = port;
  }
  else {
    link->server.single.host = host;
    link->server.single.port = port;
  }

  return SUCCESS;
}

/* {{{ Mongo->close()
 */
PHP_METHOD(Mongo, close) {
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);

  zend_list_delete(Z_LVAL_P(zlink));

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 0 TSRMLS_CC);
  zend_update_property_null(mongo_ce_Mongo, getThis(), "connection", strlen("connection") TSRMLS_CC);
  RETURN_TRUE;
}
/* }}} */


/* {{{ Mongo->__toString()
 */
PHP_METHOD(Mongo, __toString) {
  char *server;
  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);
  zval *zconn = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);

  // if we haven't connected yet, there's no link
  if (!Z_BVAL_P(zconn)) {
    zval *s = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
    RETURN_ZVAL(s, 1, 0);
  }

  // if there is a link, get the real server string
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection);

  if (link->paired) {
    spprintf(&server, 0, "%s:%d,%s:%d", link->server.paired.left, link->server.paired.lport, link->server.paired.right, link->server.paired.rport);
  }
  else {
    spprintf(&server, 0, "%s:%d", link->server.single.host, link->server.single.port);
  }
  RETURN_STRING(server, 0);
}
/* }}} */


/* {{{ Mongo->selectDB()
 */
PHP_METHOD(Mongo, selectDB) {
  zval temp;
  zval *db;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &db) == FAILURE) {
    return;
  }

  object_init_ex(return_value, mongo_ce_DB);
  MONGO_METHOD(MongoDB, __construct, &temp, return_value, 2, getThis(), db);
}
/* }}} */

/* {{{ Mongo::selectCollection()
 */
PHP_METHOD(Mongo, selectCollection) {
  zval *db, *collection, *temp_db;
  mongo_db *ok;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &db, &collection) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(db) != IS_OBJECT ||
      Z_OBJCE_P(db) != mongo_ce_DB) {
    MAKE_STD_ZVAL(temp_db);

    // reusing db param from Mongo::selectCollection call...
    // a little funky, but kind of clever
    MONGO_METHOD_BASE(Mongo, selectDB)(1, temp_db, NULL, getThis(), 0 TSRMLS_CC);

    ok = (mongo_db*)zend_object_store_get_object(temp_db TSRMLS_CC);
    MONGO_CHECK_INITIALIZED(ok->name, MongoDB);

    db = temp_db;
  }
  else {
    zval_add_ref(&db);
  }

  MONGO_METHOD(MongoDB, selectCollection, return_value, db, 1, collection);
  zval_ptr_dtor(&db);
}
/* }}} */

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

  MONGO_METHOD(MongoDB, drop, return_value, db, 0);
  zval_ptr_dtor(&db);
}
/* }}} */


static void run_err(int err_type, zval *return_value, zval *this_ptr TSRMLS_DC) {
  zval *db_name, *db;
  MAKE_STD_ZVAL(db_name);
  ZVAL_STRING(db_name, "admin", 1);

  MAKE_STD_ZVAL(db);
  MONGO_METHOD(Mongo, selectDB, db, getThis(), 1, db_name);
  zval_ptr_dtor(&db_name);

  switch (err_type) {
  case LAST_ERROR:
    MONGO_METHOD(MongoDB, lastError, return_value, db, 0);
    break;
  case PREV_ERROR:
    MONGO_METHOD(MongoDB, prevError, return_value, db, 0);
    break;
  case RESET_ERROR:
    MONGO_METHOD(MongoDB, resetError, return_value, db, 0);
    break;
  case FORCE_ERROR:
    MONGO_METHOD(MongoDB, forceError, return_value, db, 0);
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
 * Takes any type of PHP var and turns it into BSON
 */
PHP_FUNCTION(bson_encode) {
  zval *z;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z) == FAILURE) {
    return;
  }

  switch (Z_TYPE_P(z)) {
  case IS_NULL: {
    RETURN_STRING("", 1);
    break;
  }
  case IS_LONG: {
    char buf[5];
    buf[4] = (char)0;
    memcpy(buf, &Z_LVAL_P(z), 4);
    RETURN_STRINGL(buf, 4, 1);
    break;
  }
  case IS_DOUBLE: {
    char buf[9];
    buf[8] = (char)0;
    memcpy(buf, &Z_LVAL_P(z), 8);
    RETURN_STRINGL(buf, 8, 1);
    break;
  }
  case IS_BOOL: {
    if (Z_BVAL_P(z)) {
      RETURN_STRINGL("\x01", 1, 1);
    }
    else {
      RETURN_STRINGL("\x00", 1, 1);
    }
    break;
  }
  case IS_STRING: {
    RETURN_STRINGL(Z_STRVAL_P(z), Z_STRLEN_P(z), 1);
    break;
  }
  case IS_OBJECT: {
    buffer buf;
    zend_class_entry *clazz = Z_OBJCE_P(z);
    if (clazz == mongo_ce_Id) {
      mongo_id *id = (mongo_id*)zend_object_store_get_object(z TSRMLS_CC);
      RETURN_STRINGL(id->id, 12, 1);
      break;
    }
    else if (clazz == mongo_ce_Date) {
      CREATE_BUF(buf, 9);
      buf.pos[8] = (char)0;

      php_mongo_serialize_date(&buf, z TSRMLS_CC);
      RETURN_STRINGL(buf.start, 8, 0);
      break;
    }
    else if (clazz == mongo_ce_Regex) {
      CREATE_BUF(buf, 128);

      php_mongo_serialize_regex(&buf, z TSRMLS_CC);
      RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
      efree(buf.start);
      break;
    }
    else if (clazz == mongo_ce_Code) {
      CREATE_BUF(buf, INITIAL_BUF_SIZE);

      php_mongo_serialize_code(&buf, z TSRMLS_CC);
      RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
      efree(buf.start);
      break;
    }
    else if (clazz == mongo_ce_BinData) {
      CREATE_BUF(buf, INITIAL_BUF_SIZE);

      php_mongo_serialize_bin_data(&buf, z TSRMLS_CC);
      RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
      efree(buf.start);
      break;
    }
    else if (clazz == mongo_ce_Timestamp) {
      CREATE_BUF(buf, 9);
      buf.pos[8] = (char)0;

      php_mongo_serialize_bin_data(&buf, z TSRMLS_CC);
      RETURN_STRINGL(buf.start, 8, 0);
      break;
    }
  }
  /* fallthrough for a normal obj */
  case IS_ARRAY: {
    buffer buf;
    CREATE_BUF(buf, INITIAL_BUF_SIZE);
    zval_to_bson(&buf, HASH_P(z), 0 TSRMLS_CC);

    RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
    efree(buf.start);
    break;
  }
  default:
#if ZEND_MODULE_API_NO >= 20060613
    zend_throw_exception(zend_exception_get_default(TSRMLS_C), "couldn't serialize element", 0 TSRMLS_CC);
#else
    zend_throw_exception(zend_exception_get_default(), "couldn't serialize element", 0 TSRMLS_CC);
#endif
    return;
  }
}

/*
 * Takes a serialized BSON object and turns it into a PHP array.
 * This only deserializes entire documents!
 */
PHP_FUNCTION(bson_decode) {
  zval *result;
  char *str;
  int str_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
    return;
  }

  array_init(return_value);
  bson_to_zval(str, HASH_P(return_value) TSRMLS_CC);
}


static int get_master(mongo_link *link TSRMLS_DC) {
  zval temp_ret;
  zval *cursor_zval, *query, *is_master, *response;
  zval **ans;
  mongo_link temp;
  mongo_cursor *cursor;

  if (!link->paired) {
    return link->server.single.socket;
  }

  if (link->server.paired.lsocket == link->master &&
      link->server.paired.lconnected) {
    return link->server.paired.lsocket;
  }
  else if (link->server.paired.rsocket == link->master &&
           link->server.paired.rconnected) {
    return link->server.paired.rsocket;
  }

  MAKE_STD_ZVAL(cursor_zval);
  object_init_ex(cursor_zval, mongo_ce_Cursor);
  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);

  // redetermine master
  MAKE_STD_ZVAL(query);
  array_init(query);
  MAKE_STD_ZVAL(is_master);
  array_init(is_master);
  add_assoc_long(is_master, "ismaster", 1);
  add_assoc_zval(query, "query", is_master);

  cursor->ns = estrdup("admin.$cmd");
  cursor->query = query;
  cursor->fields = 0;
  cursor->limit = -1;
  cursor->skip = 0;
  cursor->opts = 0;

  temp.paired = 0;

  MAKE_STD_ZVAL(response);
  if (link->server.paired.lconnected) {
    // check the left
    temp.server.single.socket = link->server.paired.lsocket;
    cursor->link = &temp;

    // need to call this after setting cursor->link
    // reset checks that cursor->link != 0
    MONGO_METHOD(MongoCursor, reset, &temp_ret, cursor_zval, 0);
    MONGO_METHOD(MongoCursor, getNext, response, cursor_zval, 0);
    if ((Z_TYPE_P(response) == IS_ARRAY ||
         Z_TYPE_P(response) == IS_OBJECT) &&
        zend_hash_find(HASH_P(response), "ismaster", 9, (void**)&ans) == SUCCESS &&
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&cursor_zval);
      zval_ptr_dtor(&query);
      zval_ptr_dtor(&response);
      return link->master = link->server.paired.lsocket;
    }

    // reset response
    zval_ptr_dtor(&response);
    MAKE_STD_ZVAL(response);
  }

  if (link->server.paired.rconnected) {
    // check the right
    temp.server.single.socket = link->server.paired.rsocket;
    cursor->link = &temp;

    MONGO_METHOD(MongoCursor, reset, &temp_ret, cursor_zval, 0);
    MONGO_METHOD(MongoCursor, getNext, response, cursor_zval, 0);
    if ((Z_TYPE_P(response) == IS_ARRAY ||
         Z_TYPE_P(response) == IS_OBJECT) &&
        zend_hash_find(HASH_P(response), "ismaster", 9, (void**)&ans) == SUCCESS &&
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&cursor_zval);
      zval_ptr_dtor(&query);
      zval_ptr_dtor(&response);
      return link->master = link->server.paired.rsocket;
    }
  }

  zval_ptr_dtor(&response);
  zval_ptr_dtor(&query);
  zval_ptr_dtor(&cursor_zval);
  return FAILURE;
}


int php_mongo_get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC) {
  int sock = get_master(cursor->link TSRMLS_CC);
  int num_returned = 0;

  if (check_connection(cursor->link, errmsg TSRMLS_CC) != SUCCESS) {
    ZVAL_STRING(errmsg, "could not establish db connection", 1);
    return FAILURE;
  }

  // if this fails, we might be disconnected... but we're probably
  // just out of results
  if (recv(sock, (char*)&cursor->header.length, INT_32, FLAGS) == FAILURE) {

    set_disconnected(cursor->link);

    /* set the error message to NULL so we know this isn't a "real" error */
    ZVAL_NULL(errmsg);
    return FAILURE;
  }

  // make sure we're not getting crazy data
  if (cursor->header.length == 0) {

    set_disconnected(cursor->link);

    /* copy message to the heap */
    ZVAL_STRING(errmsg, "no db response", 1);
    return FAILURE;
  }
  else if (cursor->header.length > MAX_RESPONSE_LEN ||
           cursor->header.length < REPLY_HEADER_SIZE) {
    char *msg;

    set_disconnected(cursor->link);

    spprintf(&msg, 
             0, 
             "bad response length: %d, max: %d, did the db assert?", 
             cursor->header.length, 
             MAX_RESPONSE_LEN);

    /* the message is already on the heap, so don't copy it again */
    ZVAL_STRING(errmsg, msg, 0);
    return FAILURE;
  }

  if (recv(sock, (char*)&cursor->header.request_id, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->header.response_to, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->header.op, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->flag, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->cursor_id, INT_64, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->start, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&num_returned, INT_32, FLAGS) == FAILURE) {

    ZVAL_STRING(errmsg, "incomplete response", 1);
    return FAILURE;
  }

  // create buf
  cursor->header.length -= INT_32*9;

  // point buf.start at buf's first char
  if (!cursor->buf.start) {
    cursor->buf.start = (unsigned char*)emalloc(cursor->header.length);
    cursor->buf.end = cursor->buf.start + cursor->header.length;
  }
  /* if we've already got a buffer allocated but it's too small, resize it
   * 
   * TODO: profile with a large response
   * this might actually be slower than freeing/reallocating, as it might
   * have to copy over all the bytes if there isn't contiguous free space.  
   */
  else if (cursor->buf.end - cursor->buf.start < cursor->header.length) {
    cursor->buf.start = (unsigned char*)erealloc(cursor->buf.start, cursor->header.length);
    cursor->buf.end = cursor->buf.start + cursor->header.length;
  }
  cursor->buf.pos = cursor->buf.start;

  /* get the actual response content */
  if (mongo_hear(cursor->link, cursor->buf.pos, cursor->header.length TSRMLS_CC) == FAILURE) {
    char *msg;
#ifdef WIN32
    spprintf(&msg, 0, "WSA error getting database response: %d", WSAGetLastError());
#else
    spprintf(&msg, 0, "error getting database response: %s", strerror(errno));
#endif
    ZVAL_STRING(errmsg, msg, 0);
    return FAILURE;
  }

  /* cursor->num is the total of the elements we've retrieved
   * (elements already iterated through + elements in db response
   * but not yet iterated through) 
   */
  cursor->num += num_returned;

  /* if no catastrophic error has happened yet, we're fine, set errmsg to null */
  ZVAL_NULL(errmsg);
  return num_returned == 0 ? FAILURE : SUCCESS;
}


int mongo_say(mongo_link *link, buffer *buf, zval *errmsg TSRMLS_DC) {
  int sock, sent;
  sock = get_master(link TSRMLS_CC);
  sent = send(sock, (const char*)buf->start, buf->pos-buf->start, FLAGS);

  if (sent == FAILURE) {
    set_disconnected(link);

    if (check_connection(link, errmsg TSRMLS_CC) == SUCCESS) {
      sock = get_master(link TSRMLS_CC);
      sent = send(sock, (const char*)buf->start, buf->pos-buf->start, FLAGS);
    }
    else {
      return FAILURE;
    }
  }

  return sent;
}

int mongo_hear(mongo_link *link, void *dest, int len TSRMLS_DC) {
  int num = 1, r = 0;

  // this can return FAILED if there is just no more data from db
  while(r < len && num > 0) {

#ifdef WIN32
    // windows gives a WSAEFAULT if you try to get more bytes
    num = recv(get_master(link TSRMLS_CC), (char*)dest, 4096, FLAGS);
#else
    num = recv(get_master(link TSRMLS_CC), (char*)dest, len, FLAGS);
#endif

    if (num < 0) {
      return FAILURE;
    }

    dest = (char*)dest + num;
    r += num;
  }
  return r;
}

static int check_connection(mongo_link *link, zval *errmsg TSRMLS_DC) {
  int now = time(0);

  if (!MonGlo(auto_reconnect) ||
      (!link->paired && link->server.single.connected) ||
      (link->paired && (link->server.paired.lconnected || link->server.paired.rconnected))) {
    return SUCCESS;
  }

  link->ts = now;

#ifdef WIN32
  if (link->paired) {
    closesocket(link->server.paired.lsocket);
    closesocket(link->server.paired.rsocket);
  }
  else {
    closesocket(link->server.single.socket);
  }
  WSACleanup();
#else
  if (link->paired) {
    close(link->server.paired.lsocket);
    close(link->server.paired.rsocket);
  }
  else {
    close(link->server.single.socket);
  }
#endif

  set_disconnected(link);

  return mongo_do_socket_connect(link, errmsg TSRMLS_CC);
}

inline void set_disconnected(mongo_link *link) {
  if (link->paired) {
    link->server.paired.lconnected = 0;
    link->server.paired.rconnected = 0;
  }
  else {
    link->server.single.connected = 0;
  }
}

static int mongo_connect_nonb(int sock, char *host, int port, zval *errmsg) {
  struct sockaddr_in addr, addr2;
  fd_set rset, wset;
  struct timeval tval;
  int connected = FAILURE;

#ifdef WIN32
  WORD version;
  WSADATA wsaData;
  int size, error;
  u_long no = 0;
  const char yes = 1;

  version = MAKEWORD(2,2);
  error = WSAStartup(version, &wsaData);

  if (error != 0) {
    return FAILURE;
  }

  // create socket
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    return FAILURE;
  }

#else
  uint size;
  int yes = 1;

  // create socket
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == FAILURE) {
    ZVAL_STRING(errmsg, strerror(errno), 1);
    return FAILURE;
  }
#endif

  // timeout
  tval.tv_sec = 20;
  tval.tv_usec = 0;

  // get addresses
  if (mongo_get_sockaddr(&addr, host, port, errmsg) == FAILURE) {
    return FAILURE;
  }

  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, INT_32);
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, INT_32);

#ifdef WIN32
  ioctlsocket(sock, FIONBIO, (u_long*)&yes);
#else
  fcntl(sock, F_SETFL, FLAGS|O_NONBLOCK);
#endif

  FD_ZERO(&rset);
  FD_SET(sock, &rset);
  FD_ZERO(&wset);
  FD_SET(sock, &wset);


  // connect
  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef WIN32
    errno = WSAGetLastError();
    if (errno != WSAEINPROGRESS &&
		errno != WSAEWOULDBLOCK)
#else
    if (errno != EINPROGRESS)
#endif
    {
      ZVAL_STRING(errmsg, strerror(errno), 1);      
      return FAILURE;
    }

    if (select(sock+1, &rset, &wset, NULL, &tval) == 0) {
      ZVAL_STRING(errmsg, strerror(errno), 1);      
      return FAILURE;
    }

    size = sizeof(addr2);

    connected = getpeername(sock, (struct sockaddr*)&addr, &size);
    if (connected == FAILURE) {
      ZVAL_STRING(errmsg, strerror(errno), 1);
      return FAILURE;
    }
  }

// reset flags
#ifdef WIN32
  ioctlsocket(sock, FIONBIO, &no);
#else
  fcntl(sock, F_SETFL, FLAGS);
#endif
  return sock;
}

static int mongo_do_socket_connect(mongo_link *link, zval *errmsg TSRMLS_DC) {
  int left, right;
  if (link->paired) {
    if ((link->server.paired.lsocket =
         mongo_connect_nonb(link->server.paired.lsocket,
                            link->server.paired.left,
                            link->server.paired.lport, errmsg)) == FAILURE) {

      left = 0;
    }
    else {
      left = 1;
    }
    if ((link->server.paired.rsocket =
         mongo_connect_nonb(link->server.paired.rsocket,
                            link->server.paired.right,
                            link->server.paired.rport, errmsg)) == FAILURE) {
      right = 0;
    }
    else {
      right = 1;
    }

    if (!left && !right) {
      return FAILURE;
    }

    link->server.paired.lconnected = left;
    link->server.paired.rconnected = right;

    if (get_master(link TSRMLS_CC) == FAILURE) {
      ZVAL_STRING(errmsg, "couldn't determine master", 1);      
      return FAILURE;
    }
  }
  else {
    if ((link->server.single.socket =
         mongo_connect_nonb(link->server.single.socket,
                            link->server.single.host,
                            link->server.single.port, errmsg)) == FAILURE) {
      link->server.single.connected = 0;
      return FAILURE;
    }
    link->server.single.connected = 1;
  }

  // set initial connection time
  link->ts = time(0);

  return SUCCESS;
}

static int mongo_get_sockaddr(struct sockaddr_in *addr, char *host, int port, zval *errmsg) {
  struct hostent *hostinfo;

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  hostinfo = (struct hostent*)gethostbyname(host);

  if (hostinfo == NULL) {
    char *errstr;
    spprintf(&errstr, 0, "couldn't get host info for %s", host); 
    ZVAL_STRING(errmsg, errstr, 1);
    return FAILURE;
  }

#ifdef WIN32
  addr->sin_addr.s_addr = ((struct in_addr*)(hostinfo->h_addr))->s_addr;
#else
  addr->sin_addr = *((struct in_addr*)hostinfo->h_addr);
#endif

  return SUCCESS;
}

/*
 * pushes an arbitrary number of arguments onto
 * the stack before calling a zim_Class_method
 */
void php_mongo_push_params(TSRMLS_D, int num, ...) {
  va_list list;
  int count = 0;

  // don't add anything if num is 0
  if (!num) {
    return;
  }

  va_start(list, num);
  while (count++ < num) {
    PUSH_PARAM(va_arg(list, zval*));
  }
  va_end(list);

  PUSH_PARAM((void*)num);
  PUSH_EO_PARAM();
}

/*
 * pops arguments from the stack after a
 * method has been called
 */
void php_mongo_pop_params(TSRMLS_D, int num) {
  int count = 0;

  // don't pop anything if num is 0
  if (!num) {
    return;
  }

  POP_EO_PARAM();
  while (count++ <= num) {
    POP_PARAM();
  }
}
