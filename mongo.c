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
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netdb.h>
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

extern zend_class_entry *mongo_ce_DB;

static void mongo_link_dtor(mongo_link*);
static void connect_already(INTERNAL_FUNCTION_PARAMETERS, int);
static int get_master(mongo_link* TSRMLS_DC);
static int check_connection(mongo_link* TSRMLS_DC);
static int mongo_connect_nonb(int, char*, int);
static int mongo_do_socket_connect(mongo_link* TSRMLS_DC);
static int mongo_get_sockaddr(struct sockaddr_in*, char*, int);
static void kill_cursor(mongo_cursor* TSRMLS_DC);
static void get_host_and_port(char*, mongo_link* TSRMLS_DC);
static void mongo_init_MongoExceptions(TSRMLS_D);

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
  { NULL, NULL, NULL }
}; 

static function_entry Mongo_methods[] = {
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
  PHP_ME(Mongo, repairDB, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, lastError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, prevError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, resetError, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, forceError, NULL, ZEND_ACC_PUBLIC)
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
      // close the connection
#ifdef WIN32
      closesocket(link->server.single.socket);
#else
      close(link->server.single.socket);
#endif

      // free strings
      if (link->server.single.host) {
        pefree(link->server.single.host, link->persist);
      }
    }

    if (link->username) {
      pefree(link->username, link->persist);
    }
    if (link->password) {
      pefree(link->password, link->persist);
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

  // make mongo objects uncloneable
  memcpy(&mongo_default_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mongo_default_handlers.clone_obj = NULL;

  // start random number generator
#ifdef WIN32
  srand(time(0));
#else
  uint seed;
  FILE *rand = fopen("/dev/urandom", "rb");
  char *seed_ptr = (char*)(void*)&seed;
  fgets(seed_ptr, sizeof(uint), rand);
  fclose(rand);

  srand(seed);
#endif

  return SUCCESS;
}


#if ZEND_MODULE_API_NO >= 20060613
/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mongo) {
#else
/* {{{ mongo_init_globals
 */
static void mongo_init_globals(zend_mongo_globals *mongo_globals TSRMLS_DC) {
#endif /* ZEND_MODULE_API_NO >= 20060613 */
  mongo_globals->num_persistent = 0; 
  mongo_globals->num_links = 0; 
  mongo_globals->auto_reconnect = 0; 
  mongo_globals->default_host = "localhost"; 
  mongo_globals->default_port = 27017; 
  mongo_globals->request_id = 3;
  mongo_globals->chunk_size = DEFAULT_CHUNK_SIZE;
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

  INIT_CLASS_ENTRY(ce, "Mongo", Mongo_methods);
  mongo_ce_Mongo = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_string(mongo_ce_Mongo, "DEFAULT_HOST", strlen("DEFAULT_HOST"), MonGlo(default_host) TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Mongo, "DEFAULT_PORT", strlen("DEFAULT_PORT"), MonGlo(default_port) TSRMLS_CC);

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
    MONGO_METHOD(Mongo, connect)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  }
  else {
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 0 TSRMLS_CC);
  }
}
/* }}} */


/* {{{ Mongo->connect
 */
PHP_METHOD(Mongo, connect) {
  zval *zempty;
  MAKE_STD_ZVAL(zempty);
  ZVAL_STRING(zempty, "", 1);

  PUSH_PARAM(zempty); PUSH_PARAM(zempty); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, connectUtil)(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
  zval_ptr_dtor(&zempty);
}

/* {{{ Mongo->pairConnect
 */
PHP_METHOD(Mongo, pairConnect) {
  zval *zempty;
  MAKE_STD_ZVAL(zempty);
  ZVAL_STRING(zempty, "", 1);

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), 1 TSRMLS_CC);

  PUSH_PARAM(zempty); PUSH_PARAM(zempty); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, connectUtil)(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
  zval_ptr_dtor(&zempty);
}

/* {{{ Mongo->persistConnect
 */
PHP_METHOD(Mongo, persistConnect) {
  zval *zusername, *zpassword;

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), 1 TSRMLS_CC);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &zusername, &zpassword) == FAILURE) {
    return;
  } 

  PUSH_PARAM(&zusername); PUSH_PARAM(&zpassword); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, connectUtil)(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}

/* {{{ Mongo->pairPersistConnect
 */
PHP_METHOD(Mongo, pairPersistConnect) {
  zval *zusername, *zpassword;

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), 1 TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), 1 TSRMLS_CC);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &zusername, &zpassword) == FAILURE) {
    return;
  } 

  PUSH_PARAM(&zusername); PUSH_PARAM(&zpassword); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, connectUtil)(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}


PHP_METHOD(Mongo, connectUtil) {
  zval *connected, *server;

  // if we're already connected, disconnect
  connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  if (Z_BVAL_P(connected)) {
    // Mongo->close()
    MONGO_METHOD(Mongo, close)(INTERNAL_FUNCTION_PARAM_PASSTHRU);

    // Mongo->connected = false
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  }
  connect_already(INTERNAL_FUNCTION_PARAM_PASSTHRU, NOT_LAZY);
  
  connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  // if connecting failed, throw an exception
  if (!Z_BVAL_P(connected)) {
    server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
    zend_throw_exception(mongo_ce_ConnectionException, Z_STRVAL_P(server), 0 TSRMLS_CC);
    return;
  }

  // set the Mongo->connected property
  Z_LVAL_P(connected) = 1;
}


static void connect_already(INTERNAL_FUNCTION_PARAMETERS, int lazy) {
  zval *username, *password, *server, *pair, *persist;
  mongo_link *link;
  zend_rsrc_list_entry new_le; 
  zend_rsrc_list_entry *le;
  char *key;
  int key_len, ulen, plen;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &username, &password) == FAILURE) {
    return;
  }

  server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);

  pair = zend_read_property(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), NOISY TSRMLS_CC);
  persist = zend_read_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), NOISY TSRMLS_CC);

  /* make sure that there aren't too many links already */
  if (MonGlo(max_links) > -1 &&
      MonGlo(max_links) <= MonGlo(num_links)) {
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
    RETURN_FALSE;
  }

  zend_update_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), return_value TSRMLS_CC);

  if (Z_BVAL_P(persist)) {
    key_len = spprintf(&key, 0, "%s_%s_%s", Z_STRVAL_P(server), Z_STRVAL_P(username), Z_STRVAL_P(password));
    // if a connection is found, return it 
    if (zend_hash_find(&EG(persistent_list), key, key_len+1, (void**)&le) == SUCCESS) {
      link = (mongo_link*)le->ptr;
      ZEND_REGISTER_RESOURCE(return_value, link, le_pconnection);
      efree(key);
      return;
    }
    // if lazy and no connection was found, return 
    else if(lazy) {
      efree(key);
      return;
    }
    efree(key);
  }

  if (Z_STRLEN_P(server) == 0) {
    return;
  }


  link = (mongo_link*)pemalloc(sizeof(mongo_link), Z_BVAL_P(persist));

  // zero pointers so it doesn't segfault on cleanup if 
  // connection fails
  link->username = 0;
  link->password = 0;
  link->paired = Z_BVAL_P(pair);
  link->persist = Z_BVAL_P(persist);

  get_host_and_port(Z_STRVAL_P(server), link TSRMLS_CC);
  if (mongo_do_socket_connect(link TSRMLS_CC) == FAILURE) {
    mongo_link_dtor(link);
    return;
  }

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 1 TSRMLS_CC);

  // store a reference in the persistence list
  if (Z_BVAL_P(persist)) {

    // save username and password for reconnection
    if (Z_STRLEN_P(username) > 0 && Z_STRLEN_P(password) > 0) {
      link->username = (char*)pestrdup(Z_STRVAL_P(username), link->persist);
      link->password = (char*)pestrdup(Z_STRVAL_P(password), link->persist);
    }

    key_len = spprintf(&key, 0, "%s_%s_%s", Z_STRVAL_P(server), Z_STRVAL_P(username), Z_STRVAL_P(password));
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

  MonGlo(num_links)++;
}

static void get_host_and_port(char *server, mongo_link *link TSRMLS_DC) {
  char *colon, *host, *comma;
  int port, host_len;

  // extract host:port
  colon = strchr(server, ':');
  if (colon) {
    host_len = colon-server+1;
    host = (char*)pemalloc(host_len, link->persist);
    memcpy(host, server, host_len-1);
    host[host_len-1] = 0;
    port = atoi(colon+1);
  }
  else {
    host = estrdup(server);
    port = 27017;
  }

  if (link->paired) {

    link->server.paired.left = host;
    link->server.paired.lport = port;


    // we get a string: host1:123,host2:456
    comma = strchr(server, ',');
    comma++;
    colon = strchr(comma, ':');

    // check that length is sane
    if (colon && 
        colon - comma > 0 &&
        colon - comma < 256) {
      host_len = colon-comma + 1;
      host = link->persist ? 
        zend_strndup(comma, host_len-1) : 
        estrndup(comma, host_len-1);
      port = atoi(colon + 1);
    }
    else {
      host = pestrdup(comma, link->persist);
      port = 27017;
    }

    link->server.paired.right = host;
    link->server.paired.rport = port;
  }
  else {
    link->server.single.host = host;
    link->server.single.port = port;
  }
}

/* {{{ Mongo->close() 
 */
PHP_METHOD(Mongo, close) {
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);

  zend_list_delete(Z_LVAL_P(zlink));

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 0 TSRMLS_CC);
  zend_update_property_null(mongo_ce_Mongo, getThis(), "connection", strlen("connection") TSRMLS_CC);
  RETURN_TRUE;
}
/* }}} */


/* {{{ Mongo->__toString()
 */
PHP_METHOD(Mongo, __toString) {
  zval *server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), 1 TSRMLS_CC);
  RETURN_ZVAL(server, 0, 1);
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

  PUSH_PARAM(getThis()); PUSH_PARAM(db); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, __construct)(2, &temp, NULL, return_value, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
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

    MONGO_METHOD(Mongo, selectDB)(1, temp_db, &temp_db, getThis(), return_value_used TSRMLS_CC);

    ok = (mongo_db*)zend_object_store_get_object(temp_db TSRMLS_CC);
    MONGO_CHECK_INITIALIZED(ok->name, MongoDB);

    if (!ok || !ok->name) {
      return;
    }

    db = temp_db;
  }
  else {
    zval_add_ref(&db);
  }

  PUSH_PARAM(collection); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, selectCollection)(1, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

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

    MONGO_METHOD(Mongo, selectDB)(1, temp_db, &temp_db, getThis(), return_value_used TSRMLS_CC);
    db = temp_db;
  }
  else {
    zval_add_ref(&db);
  }

  MONGO_METHOD(MongoDB, drop)(0, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);

  zval_ptr_dtor(&db);
}
/* }}} */

/* {{{ Mongo::repairDB()
 */
PHP_METHOD(Mongo, repairDB) {
  zval *db, *preserve_clones = 0, *backup = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|zz", &db, mongo_ce_DB, &preserve_clones, &backup) == FAILURE) {
    RETURN_FALSE;
  }

  if (!preserve_clones) {
    MAKE_STD_ZVAL(preserve_clones);
    ZVAL_BOOL(preserve_clones, 0);
  }
  else {
    zval_add_ref(&preserve_clones);
  }

  if (!backup) {
    MAKE_STD_ZVAL(backup);
    ZVAL_BOOL(backup, 0);
  }
  else {
    zval_add_ref(&backup);
  }
  
  PUSH_PARAM(preserve_clones); PUSH_PARAM(backup); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, selectCollection)(1, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&backup);
  zval_ptr_dtor(&preserve_clones);
}
/* }}} */

/* {{{ Mongo->lastError()
 */
PHP_METHOD(Mongo, lastError) {
  zval *name, *data;
  zval db;

  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(name); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, selectDB)(1, &db, NULL, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "getlasterror", 1);

  PUSH_PARAM(data); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, command)(1, return_value, return_value_ptr, &db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&data);
}
/* }}} */

/* {{{ Mongo->prevError()
 */
PHP_METHOD(Mongo, prevError) {
  zval *name, *data;
  zval db;

  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(name); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, selectDB)(1, &db, NULL, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "getpreverror", 1);

  PUSH_PARAM(data); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, command)(1, return_value, return_value_ptr, &db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&data);
}
/* }}} */

/* {{{ Mongo->resetError()
 */
PHP_METHOD(Mongo, resetError) {
  zval *name, *data;
  zval db;

  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(name); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, selectDB)(1, &db, NULL, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "reseterror", 1);

  PUSH_PARAM(data); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, command)(1, return_value, return_value_ptr, &db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&data);
}
/* }}} */

/* {{{ Mongo->forceError()
 */
PHP_METHOD(Mongo, forceError) {
  zval *name, *data;
  zval db;

  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(name); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(Mongo, selectDB)(1, &db, NULL, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "forceerror", 1);

  PUSH_PARAM(data); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, command)(1, return_value, return_value_ptr, &db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&data);
}
/* }}} */



mongo_cursor* mongo_do_query(mongo_link *link, char *collection, int skip, int limit, zval *zquery, zval *zfields TSRMLS_DC) {
  int sent;
  mongo_cursor *cursor;
  mongo_msg_header header;

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, collection, strlen(collection), OP_QUERY);

  serialize_int(&buf, skip);
  serialize_int(&buf, limit);

  zval_to_bson(&buf, zquery, NO_PREP TSRMLS_CC);
  if (zfields && zend_hash_num_elements(Z_ARRVAL_P(zfields)) > 0) {
    zval_to_bson(&buf, zfields, NO_PREP TSRMLS_CC);
  }

  serialize_size(buf.start, &buf);
  // sends
  sent = mongo_say(link, &buf TSRMLS_CC);
  efree(buf.start);
  if (sent == FAILURE) {
    zend_error(E_WARNING, "couldn't send query\n");
    return 0;
  }

  cursor = (mongo_cursor*)emalloc(sizeof(mongo_cursor));
  memset(cursor, 0, sizeof(mongo_cursor));
  cursor->link = link;
  cursor->ns = estrdup(collection);

  get_reply(cursor TSRMLS_CC);

  cursor->limit = limit;
  return cursor;
}


int mongo_do_has_next(mongo_cursor *cursor TSRMLS_DC) {
  mongo_msg_header header;
  buffer buf;
  int size = 34+strlen(cursor->ns);

  if (cursor->num == 0) {
    return 0;
  }
  if (cursor->at < cursor->num) {
    return 1;
  }
 
  buf.start = (unsigned char*)emalloc(size);
  buf.pos = buf.start;
  buf.end = buf.start + size;

  CREATE_RESPONSE_HEADER(buf, cursor->ns, strlen(cursor->ns), cursor->header.request_id, OP_GET_MORE);
  serialize_int(&buf, cursor->limit);
  serialize_long(&buf, cursor->cursor_id);
  serialize_size(buf.start, &buf);

  // fails if we're out of elems
  if(mongo_say(cursor->link, &buf TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return 0;
  }
  
  efree(buf.start);

  // if we have cursor->at == cursor->num && recv fails,
  // we're probably just out of results
  return (get_reply(cursor TSRMLS_CC) == SUCCESS);
}


zval* mongo_do_next(mongo_cursor *cursor TSRMLS_DC) {
  zval *elem = 0;

  if (cursor->at >= cursor->num) {
    // check for more results
    if (!mongo_do_has_next(cursor TSRMLS_CC)) {
      // we're out of results
      return 0;
    }
    // we got more results
  }

  if (cursor->at < cursor->num) {
    MAKE_STD_ZVAL(elem);
    array_init(elem);
    cursor->buf.pos = bson_to_zval(cursor->buf.pos, elem TSRMLS_CC);

    // increment cursor position
    cursor->at++;
  }
  return elem;
}


static int get_master(mongo_link *link TSRMLS_DC) {
  zval *is_master, *response;
  mongo_link temp;
  mongo_cursor *c;
  zval **ans;

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

  // redetermine master
  ALLOC_INIT_ZVAL(is_master);
  array_init(is_master);
  add_assoc_long(is_master, "ismaster", 1);

  temp.paired = 0;
  temp.server.single.socket = link->server.paired.lsocket;

  // check the left
  c = mongo_do_query(&temp, "admin.$cmd", 0, -1, is_master, 0 TSRMLS_CC);

  if ((response = mongo_do_next(c TSRMLS_CC)) != NULL) {
    if (zend_hash_find(Z_ARRVAL_P(response), "ismaster", 9, (void**)&ans) == SUCCESS && 
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&is_master);
      mongo_mongo_cursor_free(c TSRMLS_CC);
      zval_ptr_dtor(&response);
      return link->master = link->server.paired.lsocket;
    }
  }
  // cleanup
  mongo_mongo_cursor_free(c TSRMLS_CC);
  zval_ptr_dtor(&response);

  // check the right
  temp.server.single.socket = link->server.paired.rsocket;

  c = mongo_do_query(&temp, "admin.$cmd", 0, -1, is_master, 0 TSRMLS_CC);

  if ((response = mongo_do_next(c TSRMLS_CC)) != NULL) {
    if (zend_hash_find(Z_ARRVAL_P(response), "ismaster", 9, (void**)&ans) == SUCCESS && 
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&is_master);
      mongo_mongo_cursor_free(c TSRMLS_CC);
      zval_ptr_dtor(&response);
      return link->master = link->server.paired.rsocket;
    }
  }

  // give up
  mongo_mongo_cursor_free(c TSRMLS_CC);
  zval_ptr_dtor(&response);
  zval_ptr_dtor(&is_master);
  return FAILURE;
}


int get_reply(mongo_cursor *cursor TSRMLS_DC) {
  int sock = get_master(cursor->link TSRMLS_CC);
  int num_returned = 0;

  // if this fails, we might be disconnected... but we're probably
  // just out of results
  if (read(sock, &cursor->header.length, INT_32) == FAILURE) {
    return FAILURE;
  }
  
  // make sure we're not getting crazy data
  if (cursor->header.length > MAX_RESPONSE_LEN ||
      cursor->header.length < REPLY_HEADER_SIZE) {
    zend_error(E_WARNING, "bad response length: %d, did the db assert?\n", cursor->header.length);
    return FAILURE;
  }

  read(sock, &cursor->header.request_id, INT_32);
  read(sock, &cursor->header.response_to, INT_32);
  read(sock, &cursor->header.op, INT_32);
  read(sock, &cursor->flag, INT_32);
  read(sock, &cursor->cursor_id, INT_64);
  read(sock, &cursor->start, INT_32);
  read(sock, &num_returned, INT_32);

  // create buf
  cursor->header.length -= INT_32*9;

  // point buf.start at buf's first char
  if (!cursor->buf.start) {
    cursor->buf.start = (unsigned char*)emalloc(cursor->header.length);
    cursor->buf.end = cursor->buf.start + cursor->header.length;
  } 
  else if (cursor->buf.end - cursor->buf.start < cursor->header.length) {
    cursor->buf.start = (unsigned char*)erealloc(cursor->buf.start, cursor->header.length);
    cursor->buf.end = cursor->buf.start + cursor->header.length;
  }
  cursor->buf.pos = cursor->buf.start;

  if (mongo_hear(cursor->link, cursor->buf.pos, cursor->header.length TSRMLS_CC) == FAILURE) {
    zend_error(E_WARNING, "error getting database response: %s\n", strerror(errno));
    return FAILURE;
  }

  cursor->num += num_returned;
  return num_returned == 0 ? FAILURE : SUCCESS;
}


int mongo_say(mongo_link *link, buffer *buf TSRMLS_DC) {
  int sock, sent;

  sock = get_master(link TSRMLS_CC);

#ifdef WIN32
  sent = send(sock, (const char*)buf->start, buf->pos-buf->start, FLAGS);
#else
  sent = write(sock, buf->start, buf->pos-buf->start);
#endif
  
  if (sent == FAILURE) {
    if (check_connection(link TSRMLS_CC) == SUCCESS) {
      sock = get_master(link TSRMLS_CC);
#     ifdef WIN32
      sent = send(sock, (const char*)buf->start, buf->pos-buf->start, FLAGS);
#     else
      sent = send(sock, buf->start, buf->pos-buf->start, FLAGS);
#     endif
    }
    else {
      return FAILURE;
    }
  }
  
  return sent;
}

int mongo_hear(mongo_link *link, void *dest, int len TSRMLS_DC) {
  int tries = 3, num = 1, r = 0;

  // this can return FAILED if there is just no more data from db
  while(r < len && num > 0) {

    num = recv(get_master(link TSRMLS_CC), (char*)dest, len, FLAGS);

    dest = (char*)dest + num;
    r += num;
  }
  return r;
}

static int check_connection(mongo_link *link TSRMLS_DC) {
  int now;
#ifdef WIN32
  SYSTEMTIME systemTime;
  GetSystemTime(&systemTime);
  now = systemTime.wMilliseconds;
#else
  now = time(0);
#endif

  if (!MonGlo(auto_reconnect) ||
      (!link->paired && link->server.single.connected) || 
      (link->paired && 
       (link->server.paired.lconnected || 
        link->server.paired.rconnected)) ||
      (now-link->ts) < 2) {
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

  return mongo_do_socket_connect(link TSRMLS_CC);
}

static int mongo_connect_nonb(int sock, char *host, int port) {
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
    return FAILURE;
  }
#endif

  // timeout
  tval.tv_sec = 20;
  tval.tv_usec = 0;

  // get addresses
  if (mongo_get_sockaddr(&addr, host, port) == FAILURE) {
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
		errno != WSAEWOULDBLOCK) {
#else
    if (errno != EINPROGRESS) {
#endif
      return FAILURE;
    }

    if (select(sock+1, &rset, &wset, NULL, &tval) == 0) {
      return FAILURE;
    }

    size = sizeof(addr2);

    connected = getpeername(sock, (struct sockaddr*)&addr, &size);
    if (connected == FAILURE) {
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

static int mongo_do_socket_connect(mongo_link *link TSRMLS_DC) {
  int left, right;
  if (link->paired) { 
    if ((link->server.paired.lsocket =
         mongo_connect_nonb(link->server.paired.lsocket,
                            link->server.paired.left,
                            link->server.paired.lport)) == FAILURE) {

      left = 0;
    }
    else {
      left = 1;
    }
    if ((link->server.paired.rsocket =
         mongo_connect_nonb(link->server.paired.rsocket,
                            link->server.paired.right,
                            link->server.paired.rport)) == FAILURE) {
      right = 0;
    }
    else {
      right = 1;
    }

    if (!left && !right) {
      return FAILURE;
    }

    get_master(link TSRMLS_CC);
    link->server.paired.lconnected = left;
    link->server.paired.rconnected = right;
  }
  else {
    if ((link->server.single.socket = 
         mongo_connect_nonb(link->server.single.socket,
                            link->server.single.host,
                            link->server.single.port)) == FAILURE) {
      link->server.single.connected = 0;
      return FAILURE;
    }
    link->server.single.connected = 1;
  }

  // set initial connection time
  link->ts = time(0);

  return SUCCESS;
}

static int mongo_get_sockaddr(struct sockaddr_in *addr, char *host, int port) {
  struct hostent *hostinfo;

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  hostinfo = (struct hostent*)gethostbyname(host);

  if (hostinfo == NULL) {
    return FAILURE;
  }

#ifdef WIN32
  addr->sin_addr.s_addr = ((struct in_addr*)(hostinfo->h_addr))->s_addr;
#else
  addr->sin_addr = *((struct in_addr*)hostinfo->h_addr);
#endif

  return SUCCESS;
}

