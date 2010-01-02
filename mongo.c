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
  *mongo_ce_CursorException,
  *mongo_ce_CursorTOException,
  *mongo_ce_Cursor,
  *mongo_ce_Id,
  *mongo_ce_Date,
  *mongo_ce_Regex,
  *mongo_ce_Code,
  *mongo_ce_BinData,
  *mongo_ce_Timestamp;

static void php_mongo_link_free(void* TSRMLS_DC);
static void connect_already(INTERNAL_FUNCTION_PARAMETERS, zval*);
static int php_mongo_get_master(mongo_link* TSRMLS_DC);
static int php_mongo_check_connection(mongo_link*, zval* TSRMLS_DC);
static int php_mongo_connect_nonb(mongo_server*, zval*);
static int php_mongo_do_socket_connect(mongo_link*, zval* TSRMLS_DC);
static int php_mongo_get_sockaddr(struct sockaddr_in*, char*, int, zval*);
static char* php_mongo_get_host(char**, int);
static int php_mongo_get_port(char**);
static void mongo_init_MongoExceptions(TSRMLS_D);
static void run_err(int, zval*, zval* TSRMLS_DC);
static int php_mongo_parse_server(zval*, zval* TSRMLS_DC);
static void set_disconnected(mongo_link *link);

zend_object_handlers mongo_default_handlers;

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
int le_pconnection;

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
#if ZEND_MODULE_API_NO <= 20060613
static
#endif
ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()


function_entry mongo_functions[] = {
  PHP_FE(bson_encode, NULL)
  PHP_FE(bson_decode, NULL)
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
STD_PHP_INI_BOOLEAN("mongo.auto_reconnect", "1", PHP_INI_SYSTEM, OnUpdateLong, auto_reconnect, zend_mongo_globals, mongo_globals)
STD_PHP_INI_BOOLEAN("mongo.allow_persistent", "1", PHP_INI_SYSTEM, OnUpdateLong, allow_persistent, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY_EX("mongo.max_persistent", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_persistent, zend_mongo_globals, mongo_globals, display_link_numbers)
STD_PHP_INI_ENTRY_EX("mongo.max_connections", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_links, zend_mongo_globals, mongo_globals, display_link_numbers)
STD_PHP_INI_ENTRY("mongo.default_host", "localhost", PHP_INI_ALL, OnUpdateString, default_host, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.default_port", "27017", PHP_INI_ALL, OnUpdateLong, default_port, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.chunk_size", "262144", PHP_INI_ALL, OnUpdateLong, chunk_size, zend_mongo_globals, mongo_globals)
STD_PHP_INI_ENTRY("mongo.cmd", "$", PHP_INI_ALL, OnUpdateString, cmd_char, zend_mongo_globals, mongo_globals)
PHP_INI_END()
/* }}} */


static void php_mongo_server_free(mongo_server_set *server_set, int persist TSRMLS_DC) {
  int i;

  if (!server_set || !server_set->num) {
    return;
  }

  for (i=0; i<server_set->num; i++) {
    mongo_server *current = server_set->server[i];

#ifdef WIN32
    closesocket(current->socket);
#else
    close(current->socket);
#endif

    if (current->host) {
      pefree(current->host, persist);
      current->host = 0;
    }

    pefree(current, persist);
    server_set->server[i] = 0;
  } /* while loop */
    
  pefree(server_set->server, persist);
  pefree(server_set, persist);

  if (!persist) {
    // decrement the total number of links
    MonGlo(num_links)--;
  }
}


/* {{{ php_mongo_link_free
 */
static void php_mongo_link_free(void *object TSRMLS_DC) {
  mongo_link *link = (mongo_link*)object;
  int persist;

  // already freed
  if (!link) {
    return;
  }

  persist = link->persist;

  // link->persist!=0 means it's either a persistent link or a copy of one
  // either way, we don't want to deallocate the memory yet
  if (!persist) {
    php_mongo_server_free(link->server_set, 0 TSRMLS_CC);
  }

  if (link->username) {
    efree(link->username);
    link->username = 0;
  }
  if (link->password) {
    efree(link->password);
    link->password = 0;
  }

  zend_object_std_dtor(&link->std TSRMLS_CC);

  // free connection, which is always a non-persistent struct
  efree(link);
}
/* }}} */

/* {{{ php_mongo_link_pfree
 */
static void php_mongo_link_pfree( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo_server_set *server_set = (mongo_server_set*)rsrc->ptr;
  php_mongo_server_free(server_set, 1 TSRMLS_CC);

  // if it's a persistent connection, decrement the
  // number of open persistent links
  MonGlo(num_persistent)--;

  rsrc->ptr = 0;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mongo) {
  zend_class_entry max_key, min_key;

#if ZEND_MODULE_API_NO < 20060613
  ZEND_INIT_MODULE_GLOBALS(mongo, mongo_init_globals, NULL);
#endif /* ZEND_MODULE_API_NO < 20060613 */

  REGISTER_INI_ENTRIES();

  le_pconnection = zend_register_list_destructors_ex(NULL, php_mongo_link_pfree, PHP_CONNECTION_RES_NAME, module_number);

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
  mongo_globals->auto_reconnect = 1;
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
  zend_declare_class_constant_string(mongo_ce_Mongo, "VERSION", strlen("VERSION"), "1.0.2" TSRMLS_CC);

  /* Mongo fields */
  zend_declare_property_bool(mongo_ce_Mongo, "connected", strlen("connected"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);

  zend_declare_property_null(mongo_ce_Mongo, "server", strlen("server"), ZEND_ACC_PROTECTED TSRMLS_CC);

  zend_declare_property_null(mongo_ce_Mongo, "username", strlen("username"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Mongo, "password", strlen("password"), ZEND_ACC_PROTECTED TSRMLS_CC);

  zend_declare_property_null(mongo_ce_Mongo, "persistent", strlen("persistent"), ZEND_ACC_PROTECTED TSRMLS_CC);
}

/*
 * this deals with the new mongo connection format:
 * mongodb://username:password@host:port,host:port
 */
static int php_mongo_parse_server(zval *this_ptr, zval *errmsg TSRMLS_DC) {
  int num_servers = 1;
  zval *hosts_z, *persist_z;
  char *hosts, *current, *comma;
  zend_bool persist;
  mongo_link *link;

  hosts_z = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
  hosts = Z_STRLEN_P(hosts_z) ? Z_STRVAL_P(hosts_z) : 0;
  current = hosts;

  persist_z = zend_read_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), NOISY TSRMLS_CC);
  persist = Z_TYPE_P(persist_z) == IS_STRING;

  link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  /* zero pointers so it doesn't segfault on cleanup if connection fails */
  link->ts = 0;
  link->username = 0;
  link->password = 0;
  // assume a non-persistent connection for now, we can change it soon
  link->persist = persist;

  // go with the default setup:
  // one connection to localhost:27017
  if (!hosts) {
    // set the top-level server set fields
    link->server_set = (mongo_server_set*)pemalloc(sizeof(mongo_server_set), persist);
    link->server_set->num = 1;
    link->server_set->rsrc = 0;
    link->server_set->master = -1;

    // allocate one server
    link->server_set->server = (mongo_server**)pemalloc(sizeof(mongo_server*), persist);
    link->server_set->server[0] = (mongo_server*)pemalloc(sizeof(mongo_server), persist);

    link->server_set->server[0]->host = pestrdup(MonGlo(default_host), persist);
    link->server_set->server[0]->port = MonGlo(default_port);
    link->server_set->server[0]->connected = 0;
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
    if (at) {
      link->username = estrndup(current, colon-current);
      link->password = estrndup(colon+1, at-(colon+1));

      zend_update_property_stringl(mongo_ce_Mongo, this_ptr, "username", strlen("username"), link->username, strlen(link->username) TSRMLS_CC);
      zend_update_property_stringl(mongo_ce_Mongo, this_ptr, "password", strlen("password"), link->password, strlen(link->password) TSRMLS_CC);

      // move current
      // mongodb://user:pass@host:port,host:port
      //                     ^
      current = at+1;
    }
  }

  // now we're doing the same thing, regardless of prefix
  // host1[:27017][,host2[:27017]]+

  // count the number of servers we'll be parsing so we can allocate the
  // memory (thanks, c!)
  comma = strchr(current, ',');
  while (comma) {
    num_servers++;
    comma = strchr(comma+1, ',');
  }

  // allocate the server ptr
  link->server_set = (mongo_server_set*)pemalloc(sizeof(mongo_server_set), persist);
  // allocate the top-level server set fields
  link->server_set->rsrc = 0;
  link->server_set->num = 0;
  link->server_set->master = -1;

  // allocate the number of servers we'll need
  link->server_set->server = (mongo_server**)pemalloc(sizeof(mongo_server*) * num_servers, persist);

  // current is now pointing at the first server name
  while (*current) {
    mongo_server *server;
    char *host;
    int port;

    // if there's an error, the dtor will try to free this server unless it's 0
    link->server_set->server[link->server_set->num] = 0;

    // localhost:1234
    // localhost,
    // localhost
    // ^
    if ((host = php_mongo_get_host(&current, persist)) == 0) {
      char *msg;
      spprintf(&msg, 0, "failed to get host from %s of %s", current, hosts);
      ZVAL_STRING(errmsg, msg, 0);

      return FAILURE;
    } 

    // localhost:27017
    //          ^
    if ((port = php_mongo_get_port(&current)) == 0) {
      char *msg;
      spprintf(&msg, 0, "failed to get port from %s of %s", current, hosts);
      ZVAL_STRING(errmsg, msg, 0);

      efree(host);
      return FAILURE;
    }

    // create a struct for this server
    server = (mongo_server*)pemalloc(sizeof(mongo_server), persist);
    server->host = host;
    server->port = port;
    server->socket = 0;
    server->connected = 0;

    link->server_set->server[link->server_set->num] = server;
    link->server_set->num++;

    // localhost,
    // localhost
    //          ^
    if (*current == ',') {
      current++;
    }
  }

  return SUCCESS;
}

/* {{{ Mongo->__construct
 */
PHP_METHOD(Mongo, __construct) {
  char *server = 0;
  int server_len = 0;
  zend_bool connect = 1, garbage = 0, persist = 0;
  zval *options = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|szbb", &server, &server_len, &options, &persist, &garbage) == FAILURE) {
    return;
  }

  /* new format */
  if (options) {
    if (!IS_SCALAR_P(options)) {
      zval **connect_z, **persist_z;
      if (zend_hash_find(HASH_P(options), "connect", strlen("connect")+1, (void**)&connect_z) == SUCCESS) {
        connect = Z_BVAL_PP(connect_z);
      }
      if (zend_hash_find(HASH_P(options), "persist", strlen("persist")+1, (void**)&persist_z) == SUCCESS) {
        convert_to_string(*persist_z);
        zend_update_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), *persist_z TSRMLS_CC);
        zval_add_ref(persist_z);
      }
    }
    else {
      /* backwards compatibility */
      connect = Z_BVAL_P(options);
      if (persist) {
        zend_update_property_string(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), "" TSRMLS_CC);
      }
    }
  }

  /*
   * If someone accidently does something like $hst instead of $host, we'll get
   * the empty string.
   */
  if (server && strlen(server) == 0) {
    zend_throw_exception(mongo_ce_ConnectionException, "no server name given", 0 TSRMLS_CC);
  }
  zend_update_property_stringl(mongo_ce_Mongo, getThis(), "server", strlen("server"), server, server_len TSRMLS_CC);

  if (connect) {
    /*
     * We are calling:
     *
     * $m->connect();
     *
     * so we don't need any parameters.  We've already set up the environment 
     * above.
     */
    MONGO_METHOD(Mongo, connect, return_value, getThis());
  }
  else {
    /* if we aren't connecting, set Mongo::connected to false and return */
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 0 TSRMLS_CC);
  }
} 
/* }}} */


/* {{{ Mongo->connect
 */
PHP_METHOD(Mongo, connect) {
  MONGO_METHOD(Mongo, connectUtil, return_value, getThis());
}

/* {{{ Mongo->pairConnect
 *
 * [DEPRECATED - use mongodb://host1,host2 syntax]
 */
PHP_METHOD(Mongo, pairConnect) {
  MONGO_METHOD(Mongo, connectUtil, return_value, getThis());
}

/* {{{ Mongo->persistConnect
 */
PHP_METHOD(Mongo, persistConnect) {
  zval *id = 0, *garbage = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &id, &garbage) == FAILURE) {
    return;
  }

  if (id) {
    zend_update_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), id TSRMLS_CC);
  }
  else {
    zend_update_property_string(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), "" TSRMLS_CC);
  }

  /* 
   * pass through any parameters Mongo::persistConnect got 
   * we can't use MONGO_METHOD because we don't want
   * to pop the parameters, yet.
   */
  MONGO_METHOD_BASE(Mongo, connectUtil)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* {{{ Mongo->pairPersistConnect
 *
 * [DEPRECATED - use mongodb://host1,host2 syntax]
 */
PHP_METHOD(Mongo, pairPersistConnect) {
  zval *id = 0, *garbage = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &id, &garbage) == FAILURE) {
    return;
  }

  if (id) {
    zend_update_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), id TSRMLS_CC);
  }
  else {
    zend_update_property_string(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), "" TSRMLS_CC);
  }

  /* 
   * pass through any parameters Mongo::pairPersistConnect got 
   * we can't use MONGO_METHOD because we don't want
   * to pop the parameters, yet.
   */
  MONGO_METHOD_BASE(Mongo, connectUtil)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}


PHP_METHOD(Mongo, connectUtil) {
  zval *connected, *server, *errmsg, *username, *password;

  /* initialize and clear the error message */
  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  /* if we're already connected, disconnect */
  connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  if (Z_BVAL_P(connected)) {
    // Mongo->close()
    MONGO_METHOD(Mongo, close, return_value, getThis());

    // Mongo->connected = false
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  }

  /* try to actually connect */
  connect_already(INTERNAL_FUNCTION_PARAM_PASSTHRU, errmsg);

  /* find out if we're connected */
  connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);

  /* if connecting failed, throw an exception */
  if (!Z_BVAL_P(connected)) {
    char *full_error;
    server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);

    spprintf(&full_error, 0, "%s: %s", Z_STRVAL_P(server), Z_STRVAL_P(errmsg));
    zend_throw_exception(mongo_ce_ConnectionException, full_error, 0 TSRMLS_CC);

    zval_ptr_dtor(&errmsg);
    efree(full_error);
    return;
  }

  /* try to authenticate the session, if username and password were given */
  username = zend_read_property(mongo_ce_Mongo, getThis(), "username", strlen("username"), NOISY TSRMLS_CC);
  password = zend_read_property(mongo_ce_Mongo, getThis(), "password", strlen("password"), NOISY TSRMLS_CC);

  if (Z_TYPE_P(username) == IS_STRING && Z_TYPE_P(password) == IS_STRING) {
    zval *db, *admin, *ok;
    int logged_in = 0;
    MAKE_STD_ZVAL(db);

    // get admin db
    MAKE_STD_ZVAL(admin);
    ZVAL_STRING(admin, "admin", 1);
    MONGO_METHOD1(Mongo, selectDB, db, getThis(), admin);
    zval_ptr_dtor(&admin);

    // log in
    MAKE_STD_ZVAL(ok);
    MONGO_METHOD2(MongoDB, authenticate, ok, db, username, password);

    zval_ptr_dtor(&db);

    if (Z_TYPE_P(ok) == IS_ARRAY) {
      zval **status;
      if (zend_hash_find(HASH_P(ok), "ok", strlen("ok")+1, (void**)&status) == SUCCESS) {
        logged_in = (int)Z_DVAL_PP(status);
      }
    } 
    else {
      logged_in = Z_BVAL_P(ok);
    }

    // check if we've logged in successfully
    if (!logged_in) {
      char *full_error;
      spprintf(&full_error, 0, "Couldn't authenticate with database admin: username [%s], password [%s]", Z_STRVAL_P(username), Z_STRVAL_P(password));

      zval_ptr_dtor(&ok);

      zend_throw_exception(mongo_ce_ConnectionException, full_error, 0 TSRMLS_CC);
      return;
    }

    // successfully logged in
    zval_ptr_dtor(&ok);
  }

  zval_ptr_dtor(&errmsg);

  /* set the Mongo->connected property */
  Z_LVAL_P(connected) = 1;
}


static void connect_already(INTERNAL_FUNCTION_PARAMETERS, zval *errmsg) {
  zval *persist, *server;
  mongo_link *link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  persist = zend_read_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), NOISY TSRMLS_CC);
  server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);

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
    ZVAL_NULL(persist);
  }
  /* make sure that there aren't too many persistent links already */
  if (Z_TYPE_P(persist) == IS_STRING &&
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

  /* 
   * if we're trying to make a persistent connection, check if one already
   * exists 
   */
  if (Z_TYPE_P(persist) == IS_STRING) {
    zend_rsrc_list_entry *le;
    char *key;

    spprintf(&key, 0, "%s%s", Z_STRVAL_P(server), Z_STRVAL_P(persist));

    /* if a connection is found, return it */
    if (zend_hash_find(&EG(persistent_list), key, strlen(key)+1, (void**)&le) == SUCCESS) {
      link->server_set = (mongo_server_set*)le->ptr;
      link->persist = 1;

      // set vars after reseting prop table
      zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 1 TSRMLS_CC);

      efree(key);
      return;
    }

    /* if it isn't found, just clean up and keep going */
    efree(key);
  }

  /* parse the server name given
   *
   * we can't do this earlier because, if this is a persistent connection, all 
   * the fields will be overwritten (memleak) in the block above
   */
  if (php_mongo_parse_server(getThis(), errmsg TSRMLS_CC) == FAILURE) {
    // errmsg set in parse_server
    return;
  }

  /* do the actual connection */
  if (php_mongo_do_socket_connect(link, errmsg TSRMLS_CC) == FAILURE) {
    // errmsg set in do_socket_connect
    return;
  }

  /* Mongo::connected = true */
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 1 TSRMLS_CC);

  /* 
   * if we're doing a persistent connection, store a reference in the 
   * persistence list
   */
  if (Z_TYPE_P(persist) == IS_STRING) {
    zend_rsrc_list_entry new_le;
    char *key;
    int rsrc = 0, count = 0;

    /* save id for reconnection */
    spprintf(&key, 0, "%s%s", Z_STRVAL_P(server), Z_STRVAL_P(persist));

    Z_TYPE(new_le) = le_pconnection;
    new_le.ptr = link->server_set;

    if (zend_hash_update(&EG(persistent_list), key, strlen(key)+1, (void*)&new_le, sizeof(zend_rsrc_list_entry), NULL)==FAILURE) {
      zend_throw_exception(mongo_ce_ConnectionException, "could not store persistent link", 0 TSRMLS_CC);

      php_mongo_server_free(link->server_set, 1 TSRMLS_CC);
      efree(key);
      RETURN_FALSE;
    }
    efree(key);

    link->server_set->rsrc = ZEND_REGISTER_RESOURCE(NULL, link->server_set, le_pconnection);
    link->persist = 1;

    MonGlo(num_persistent)++;
  }

  /* otherwise, just return the connection */

  MonGlo(num_links)++;
}

// get the next host from the server string
static char* php_mongo_get_host(char **ip, int persist) {
  char *colon = strchr(*ip, ':');
  char *comma = strchr(*ip, ',');
  char *end = 0;
  char *retval;

  // pick whichever exists and is sooner: ':', ',', or '\0' 

  // localhost,localhost
  // localhost,localhost:27017
  if (!colon || (colon && comma && (comma - *ip < colon - *ip))) {
    end = comma;
  }
  // ,whatever
  // :whatever
  else if ((comma && comma == *ip) || (colon && colon == *ip)) {
    return 0;
  }
  // localhost:27017,localhost
  else if (colon) {
    end = colon;
  }

  if (end) {
    // sanity check
    if (end - *ip > 1 && end - *ip < 256) {
      int len = end-*ip;

      // return a copy
      retval = persist ? zend_strndup(*ip, len) : estrndup(*ip, len);

      // move to the end of this section of string
      *(ip) = end;

      return retval;
    }
    else {
      // you get nothing
      return 0;
    }
  }

  // otherwise, this is the last thing in the string
  retval = pestrdup(*ip, persist);

  // move to the end of this string
  *(ip) = *ip + strlen(*ip);

  return retval;
}

// get the next port from the server string
static int php_mongo_get_port(char **ip) {
  int i, retval;
  /* 
   * end: ptr to the end of this port
   * host1:27017,host2:27017
   *            ^end
   * host1:27017
   *            ^end
   */
  char *end = strchr(*ip, ',');
  end = end ? end : *ip+strlen(*ip);

  // there might not even be a port
  if (**ip != ':') {
    return 27017;
  }

  // if there is, move past the colon
  // localhost:27017
  //          ^
  (*ip)++;

  // make sure the port is actually a number
  for (i = 0; i < (end - *ip); i++) {
    if ((*ip)[i] < '0' || (*ip)[i] > '9') {
      return 0;
    }
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
  mongo_link *link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(link->server_set, Mongo);

  set_disconnected(link);

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 0 TSRMLS_CC);
  RETURN_TRUE;
}
/* }}} */


/* {{{ Mongo->__toString()
 */
PHP_METHOD(Mongo, __toString) {
  int i;
  char str[256], *str_p;

  mongo_link *link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  // if we haven't connected yet, we should still be able to get the __toString 
  // from the server field
  if (!link->server_set) {
    zval *server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
    RETURN_ZVAL(server, 1, 0);
  }

  str_p = str;

  // TODO: list the master first

  for (i=0; i<link->server_set->num; i++) {
    char *port;

    // TODO: if this is the master, continue

    // if this is not the first one, add a comma
    if (str != str_p) {
      *(str_p) = ',';
      str_p++;
    }

    // if this host is not connected, enclose in []s: [localhost:27017]
    if (!link->server_set->server[i]->connected) {
      *(str_p) = '[';
      str_p++;
    }

    // copy host
    memcpy(str_p, link->server_set->server[i]->host, strlen(link->server_set->server[i]->host));
    str_p += strlen(link->server_set->server[i]->host);

    *(str_p) = ':';
    str_p++;

    // copy port
    spprintf(&port, 0, "%d", link->server_set->server[i]->port);
    memcpy(str_p, port, strlen(port));
    str_p += strlen(port);
    efree(port);

    // close []s
    if (!link->server_set->server[i]->connected) {
      *(str_p) = ']';
      str_p++;
    }
  }

  *(str_p) = '\0';

  RETURN_STRING(str, 1);
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
  MONGO_METHOD2(MongoDB, __construct, &temp, return_value, getThis(), db);
}
/* }}} */


/* {{{ Mongo::__get
 */
PHP_METHOD(Mongo, __get) {
  zval *name;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
    return;
  }

  // select this db
  MONGO_METHOD1(Mongo, selectDB, return_value, getThis(), name);
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

  MONGO_METHOD1(MongoDB, selectCollection, return_value, db, collection);
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

  MONGO_METHOD(MongoDB, drop, return_value, db);
  zval_ptr_dtor(&db);
}
/* }}} */


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
  char *str;
  int str_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
    return;
  }

  array_init(return_value);
  bson_to_zval(str, HASH_P(return_value) TSRMLS_CC);
}


static int php_mongo_get_master(mongo_link *link TSRMLS_DC) {
  int i;
  zval *cursor_zval, *query, *is_master;
  mongo_cursor *cursor;

  // for a single connection, return it
  if (link->server_set->num == 1) {
    return link->server_set->server[0]->socket;
  }

  // if we're still connected to master, return it
  if (link->server_set->master >= 0 && link->server_set->server[link->server_set->master]->connected) {
    return link->server_set->server[link->server_set->master]->socket;
  }

  // redetermine master

  // create a cursor
  MAKE_STD_ZVAL(cursor_zval);
  object_init_ex(cursor_zval, mongo_ce_Cursor);
  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);

  // query = { query : { ismaster : 1 } }
  MAKE_STD_ZVAL(query);
  array_init(query);

  // is_master = { ismaster : 1 }
  MAKE_STD_ZVAL(is_master);
  array_init(is_master);

  add_assoc_long(is_master, "ismaster", 1);
  add_assoc_zval(query, "query", is_master);

  // admin.$cmd.findOne({ query : { ismaster : 1 } })
  cursor->ns = estrdup("admin.$cmd");
  cursor->query = query;
  cursor->fields = 0;
  cursor->limit = -1;
  cursor->skip = 0;
  cursor->opts = 0;
  cursor->current = 0;
  cursor->timeout = 0;

  for (i=0; i<link->server_set->num; i++) {
    zval temp_ret;
    zval *response;
    zval **ans;
    mongo_link temp;
    mongo_server_set temp_server_set;
    mongo_server temp_server;
    mongo_server *temp_server_p = &temp_server;
    temp_server_set.server = &temp_server_p;
    temp.server_set = &temp_server_set;

    // skip anything we're not connected to
    if (!link->server_set->server[i]->connected) {
      continue;
    }

    // make a fake link
    temp.server_set->num = 1;
    temp.server_set->server[0] = link->server_set->server[i]; 
    cursor->link = &temp;
   
    // need to call this after setting cursor->link
    // reset checks that cursor->link != 0
    MONGO_METHOD(MongoCursor, reset, &temp_ret, cursor_zval);
    MAKE_STD_ZVAL(response);
    MONGO_METHOD(MongoCursor, getNext, response, cursor_zval);

    if (!IS_SCALAR_P(response) &&
        zend_hash_find(HASH_P(response), "ismaster", 9, (void**)&ans) == SUCCESS &&
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&cursor_zval);
      zval_ptr_dtor(&query);
      zval_ptr_dtor(&response);

      return link->server_set->server[link->server_set->master = i]->socket;
    }

    // reset response
    zval_ptr_dtor(&response);
  }

  zval_ptr_dtor(&cursor_zval);
  zval_ptr_dtor(&query);
  return FAILURE;
}

/*
 * This method reads the message header for a database response
 */
static int get_header(int sock, mongo_cursor *cursor, zval *errmsg) {
  if (recv(sock, (char*)&cursor->recv.length, INT_32, FLAGS) == FAILURE) {

    set_disconnected(cursor->link);

    ZVAL_STRING(errmsg, "couldn't get response header", 1);
    return FAILURE;
  }

  // switch the byte order, if necessary
  cursor->recv.length = MONGO_INT(cursor->recv.length);

  // make sure we're not getting crazy data
  if (cursor->recv.length == 0) {

    set_disconnected(cursor->link);

    ZVAL_STRING(errmsg, "no db response", 1);
    return FAILURE;
  }
  else if (cursor->recv.length > MAX_RESPONSE_LEN ||
           cursor->recv.length < REPLY_HEADER_SIZE) {
    char *msg;

    set_disconnected(cursor->link);

    spprintf(&msg, 
             0, 
             "bad response length: %d, max: %d, did the db assert?", 
             cursor->recv.length, 
             MAX_RESPONSE_LEN);

    ZVAL_STRING(errmsg, msg, 0);
    return FAILURE;
  }

  if (recv(sock, (char*)&cursor->recv.request_id, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->recv.response_to, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->recv.op, INT_32, FLAGS) == FAILURE) {
    ZVAL_STRING(errmsg, "incomplete header", 1);
    return FAILURE;
  }

  cursor->recv.request_id = MONGO_INT(cursor->recv.request_id);
  cursor->recv.response_to = MONGO_INT(cursor->recv.response_to);
  cursor->recv.op = MONGO_INT(cursor->recv.op); 

  return SUCCESS;
}

int php_mongo_get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC) {
  int sock = php_mongo_get_master(cursor->link TSRMLS_CC);
  int num_returned = 0;
  int64_t temp_id;
  char *cursor_ptr;

  if (php_mongo_check_connection(cursor->link, errmsg TSRMLS_CC) != SUCCESS) {
    ZVAL_STRING(errmsg, "could not establish db connection", 1);
    return FAILURE;
  }

  // set a timeout
  if (cursor->timeout && cursor->timeout > 0) {
    struct timeval timeout;
    fd_set readfds;

    timeout.tv_sec = cursor->timeout / 1000 ;
    timeout.tv_usec = (cursor->timeout % 1000) * 1000;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    select(sock+1, &readfds, NULL, NULL, &timeout);

    if (!FD_ISSET(sock, &readfds)) {
      ZVAL_NULL(errmsg);
      zend_throw_exception(mongo_ce_CursorTOException, "Cursor timed out", 0 TSRMLS_CC);
      return FAILURE;
    }
  }

  if (get_header(sock, cursor, errmsg) == FAILURE) {
    return FAILURE;
  }

  // check that this is actually the response we want
  while (cursor->send.request_id != cursor->recv.response_to) {
    // if it's not... 

    // TODO: check that the request_id isn't on async queue
    // temp: we have no async queue, so this is always true
    if (1) {
      char temp[4096];
      // throw out the rest of the headers and the response
      if (recv(sock, (char*)temp, 20, FLAGS) == FAILURE ||
          mongo_hear(cursor->link, (void*)temp, cursor->recv.length-REPLY_HEADER_LEN TSRMLS_CC) == FAILURE) {
        ZVAL_STRING(errmsg, "couldn't get response to throw out", 1);
        return FAILURE;
      }
    }

    // get the next db response
    if (get_header(sock, cursor, errmsg) == FAILURE) {
      return FAILURE;
    }
  }

  if (recv(sock, (char*)&cursor->flag, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->cursor_id, INT_64, FLAGS) == FAILURE ||
      recv(sock, (char*)&cursor->start, INT_32, FLAGS) == FAILURE ||
      recv(sock, (char*)&num_returned, INT_32, FLAGS) == FAILURE) {

    ZVAL_STRING(errmsg, "incomplete response", 1);
    return FAILURE;
  }

  cursor_ptr = (char*)&temp_id;
  mongo_memcpy(cursor_ptr, &cursor->cursor_id, INT_64);
  cursor->cursor_id = temp_id;

  cursor->flag = MONGO_INT(cursor->flag);
  cursor->start = MONGO_INT(cursor->start);
  num_returned = MONGO_INT(num_returned);

  // create buf
  cursor->recv.length -= REPLY_HEADER_LEN;

  // point buf.start at buf's first char
  if (!cursor->buf.start) {
    cursor->buf.start = (unsigned char*)emalloc(cursor->recv.length);
    cursor->buf.end = cursor->buf.start + cursor->recv.length;
  }
  /* if we've already got a buffer allocated but it's too small, resize it
   * 
   * TODO: profile with a large response
   * this might actually be slower than freeing/reallocating, as it might
   * have to copy over all the bytes if there isn't contiguous free space.  
   */
  else if (cursor->buf.end - cursor->buf.start < cursor->recv.length) {
    cursor->buf.start = (unsigned char*)erealloc(cursor->buf.start, cursor->recv.length);
    cursor->buf.end = cursor->buf.start + cursor->recv.length;
  }
  cursor->buf.pos = cursor->buf.start;

  /* get the actual response content */
  if (mongo_hear(cursor->link, cursor->buf.pos, cursor->recv.length TSRMLS_CC) == FAILURE) {
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
  sock = php_mongo_get_master(link TSRMLS_CC);
  sent = send(sock, (const char*)buf->start, buf->pos-buf->start, FLAGS);

  if (sent == FAILURE) {
    set_disconnected(link);

    if (php_mongo_check_connection(link, errmsg TSRMLS_CC) == SUCCESS) {
      sock = php_mongo_get_master(link TSRMLS_CC);
      sent = send(sock, (const char*)buf->start, buf->pos-buf->start, FLAGS);
    }
    else {
      return FAILURE;
    }
  }

  return sent;
}

int mongo_hear(mongo_link *link, void *dest, int total_len TSRMLS_DC) {
  int num = 1, received = 0;

  // this can return FAILED if there is just no more data from db
  while(received < total_len && num > 0) {
    int len = 4096 < (total_len - received) ? 4096 : total_len - received;

    // windows gives a WSAEFAULT if you try to get more bytes
    num = recv(php_mongo_get_master(link TSRMLS_CC), (char*)dest, len, FLAGS);

    if (num < 0) {
      return FAILURE;
    }

    dest = (char*)dest + num;
    received += num;
  }
  return received;
}

static int php_mongo_check_connection(mongo_link *link, zval *errmsg TSRMLS_DC) {
  int now = time(0), connected = 0, i;

  if ((link->server_set->num == 1 && link->server_set->server[0]->connected) ||
      (link->server_set->master >= 0 && link->server_set->server[link->server_set->master]->connected)) {
    connected = 1;
  }

  // if we're already connected or autoreconnect isn't set, we're all done 
  if (!MonGlo(auto_reconnect) || connected) {
    return SUCCESS;
  }

  link->ts = now;

  // close connection
  set_disconnected(link);

  return php_mongo_do_socket_connect(link, errmsg TSRMLS_CC);
}

static void set_disconnected(mongo_link *link) {
  // get the master connection
  int master = (link->server_set->num == 1) ? 0 : link->server_set->master;

  // already disconnected
  if (master == -1) {
    return;
  }

  // sever it
  link->server_set->server[master]->connected = 0;
#ifdef WIN32
  closesocket(link->server_set->server[master]->socket);
  WSACleanup();
#else
  close(link->server_set->server[master]->socket);
#endif /* WIN32 */

  link->server_set->master = -1;
}

static int php_mongo_connect_nonb(mongo_server *server, zval *errmsg) {
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
  server->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server->socket == INVALID_SOCKET) {
    ZVAL_STRING(errmsg, "Could not create socket", 1);
    return FAILURE;
  }

#else
  uint size;
  int yes = 1;

  // create socket
  if ((server->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == FAILURE) {
    ZVAL_STRING(errmsg, strerror(errno), 1);
    return FAILURE;
  }
#endif

  // timeout
  tval.tv_sec = 20;
  tval.tv_usec = 0;

  // get addresses
  if (php_mongo_get_sockaddr(&addr, server->host, server->port, errmsg) == FAILURE) {
    // errmsg set in mongo_get_sockaddr
    return FAILURE;
  }

  setsockopt(server->socket, SOL_SOCKET, SO_KEEPALIVE, &yes, INT_32);
  setsockopt(server->socket, IPPROTO_TCP, TCP_NODELAY, &yes, INT_32);

#ifdef WIN32
  ioctlsocket(server->socket, FIONBIO, (u_long*)&yes);
#else
  fcntl(server->socket, F_SETFL, FLAGS|O_NONBLOCK);
#endif

  FD_ZERO(&rset);
  FD_SET(server->socket, &rset);
  FD_ZERO(&wset);
  FD_SET(server->socket, &wset);


  // connect
  if (connect(server->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef WIN32
    errno = WSAGetLastError();
    if (errno != WSAEINPROGRESS && errno != WSAEWOULDBLOCK)
#else
    if (errno != EINPROGRESS)
#endif
    {
      ZVAL_STRING(errmsg, strerror(errno), 1);      
      return FAILURE;
    }

    if (select(server->socket+1, &rset, &wset, NULL, &tval) == 0) {
      ZVAL_STRING(errmsg, strerror(errno), 1);      
      return FAILURE;
    }

    size = sizeof(addr2);

    connected = getpeername(server->socket, (struct sockaddr*)&addr, &size);
    if (connected == FAILURE) {
      ZVAL_STRING(errmsg, strerror(errno), 1);
      return FAILURE;
    }

    // set connected
    server->connected = 1;
  }

// reset flags
#ifdef WIN32
  ioctlsocket(server->socket, FIONBIO, &no);
#else
  fcntl(server->socket, F_SETFL, FLAGS);
#endif
  return SUCCESS;
}

static int php_mongo_do_socket_connect(mongo_link *link, zval *errmsg TSRMLS_DC) {
  int i, connected = 0;

  for (i=0; i<link->server_set->num; i++) {
    if (!link->server_set->server[i]->connected) {
      int status = php_mongo_connect_nonb(link->server_set->server[i], errmsg);
      // FAILURE = -1
      // SUCCESS = 0
      connected |= (status+1);
    }
    else {
      connected = 1;
    }

    /*
     * cases where we have an error message and don't care because there's a
     * connection we can use:
     *  - if a connections fails after we have at least one working
     *  - if the first connection fails but a subsequent ones succeeds
     */
    if (connected && Z_TYPE_P(errmsg) == IS_STRING) {
      zval_ptr_dtor(&errmsg);
      MAKE_STD_ZVAL(errmsg);
      ZVAL_NULL(errmsg);
    }
  }

  // if at least one returns !FAILURE, return success
  if (!connected) {
    // error message set in mongo_connect_nonb
    return FAILURE;
  }

  if (php_mongo_get_master(link TSRMLS_CC) == FAILURE) {
    ZVAL_STRING(errmsg, "couldn't determine master", 1);      
    return FAILURE;
  }

  // set initial connection time
  link->ts = time(0);

  return SUCCESS;
}

static int php_mongo_get_sockaddr(struct sockaddr_in *addr, char *host, int port, zval *errmsg) {
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

