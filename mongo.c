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

#include "mongo.h"
#include "db.h"
#include "mongo_types.h"
#include "bson.h"

extern zend_class_entry *mongo_ce_DB;

static void mongo_link_dtor(mongo_link*);
static void connect_already(INTERNAL_FUNCTION_PARAMETERS, int);
static int get_master(mongo_link* TSRMLS_DC);
static int check_connection(mongo_link* TSRMLS_DC);
static int mongo_connect_nonb(int, char*, int);
static int mongo_do_socket_connect(mongo_link*);
static int get_sockaddr(struct sockaddr_in*, char*, int);
static void kill_cursor(mongo_cursor* TSRMLS_DC);
static void get_host_and_port(char*, mongo_link* TSRMLS_DC);
static void mongo_init_MongoExceptions(TSRMLS_D);

/** Classes */
zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_CursorException,
  *mongo_ce_ConnectionException,
  *mongo_ce_GridFSException,
  *mongo_ce_Exception;

/** Resources */
int le_connection, le_pconnection, le_db_cursor;

ZEND_DECLARE_MODULE_GLOBALS(mongo)
static PHP_GINIT_FUNCTION(mongo);

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
  PHP_MODULE_GLOBALS(mongo),
  PHP_GINIT(mongo),
  NULL,
  NULL,
  STANDARD_MODULE_PROPERTIES_EX
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


/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mongo){
  mongo_globals->num_persistent = 0; 
  mongo_globals->num_links = 0; 
  mongo_globals->auto_reconnect = 0; 
  mongo_globals->default_host = "localhost"; 
  mongo_globals->default_port = 27017; 
  mongo_globals->request_id = 3;
  mongo_globals->chunk_size = DEFAULT_CHUNK_SIZE;
}
/* }}} */

static void mongo_link_dtor(mongo_link *link) {
  if (link) {
    if (link->paired) {
      close(link->server.paired.lsocket);
      close(link->server.paired.rsocket);

      if (link->server.paired.left) {
        efree(link->server.paired.left);
      }
      if (link->server.paired.right) {
        efree(link->server.paired.right);
      }
    }
    else {
      // close the connection
      close(link->server.single.socket);

      // free strings
      if (link->server.single.host) {
        efree(link->server.single.host);
      }
    }

    if (link->username) {
      efree(link->username);
    }
    if (link->password) {
      efree(link->password);
    }

    // free connection
    efree(link);
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

  mongo_init_MongoUtil(TSRMLS_C);

  mongo_init_MongoId(TSRMLS_C);
  mongo_init_MongoCode(TSRMLS_C);
  mongo_init_MongoRegex(TSRMLS_C);
  mongo_init_MongoDate(TSRMLS_C);
  mongo_init_MongoBinData(TSRMLS_C);

  mongo_init_MongoExceptions(TSRMLS_C);

  return SUCCESS;
}
/* }}} */


/* {{{ mongo_init_Mongo_new
 */
zend_object_value mongo_init_Mongo_new(zend_class_entry *class_type TSRMLS_DC) {
  zval tmp, obj;
  zend_object *object;

  Z_OBJVAL(obj) = zend_objects_new(&object, class_type TSRMLS_CC);
  Z_OBJ_HT(obj) = zend_get_std_object_handlers();
 
  ALLOC_HASHTABLE(object->properties);
  zend_hash_init(object->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
  zend_hash_copy(object->properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

  return Z_OBJVAL(obj);
}
/* }}} */


void mongo_init_Mongo(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "Mongo", Mongo_methods);
  ce.create_object = mongo_init_Mongo_new;
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

  zend_class_entry e;
  INIT_CLASS_ENTRY(e, "MongoException", NULL);
  mongo_ce_Exception = zend_register_internal_class_ex(&e, (zend_class_entry*)zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);

  zend_class_entry ce;
  INIT_CLASS_ENTRY(ce, "MongoCursorException", NULL);
  mongo_ce_CursorException = zend_register_internal_class_ex(&ce, mongo_ce_Exception, NULL TSRMLS_CC);

  zend_class_entry conn;
  INIT_CLASS_ENTRY(conn, "MongoConnectionException", NULL);
  mongo_ce_ConnectionException = zend_register_internal_class_ex(&conn, mongo_ce_Exception, NULL TSRMLS_CC);

  zend_class_entry e2;
  INIT_CLASS_ENTRY(e2, "MongoGridFSException", NULL);
  mongo_ce_GridFSException = zend_register_internal_class_ex(&e2, mongo_ce_Exception, NULL TSRMLS_CC);
}

/* {{{ Mongo->__construct 
 */
PHP_METHOD(Mongo, __construct) {
  char *server = 0;
  int server_len = 0;
  zend_bool connect = 1, paired = 0, persist = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sbbb", &server, &server_len, &connect, &paired, &persist) == FAILURE) {
    return;
  }

  if (!server) {
    zval *zserver = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
    server = Z_STRVAL_P(zserver);
    server_len = Z_STRLEN_P(zserver);
  }

  zend_update_property_stringl(mongo_ce_Mongo, getThis(), "server", strlen("server"), server, server_len TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), paired TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), persist TSRMLS_CC);

  if (connect) {
    zim_Mongo_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  }
  else {
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), 0 TSRMLS_CC);
  }
}
/* }}} */


/* {{{ Mongo->connect
 */
PHP_METHOD(Mongo, connect) {
  zval zusername, zpassword;
  ZVAL_STRING(&zusername, "", 0);
  ZVAL_STRING(&zpassword, "", 0);

  PUSH_PARAM(&zusername); PUSH_PARAM(&zpassword); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_Mongo_connectUtil(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}

/* {{{ Mongo->pairConnect
 */
PHP_METHOD(Mongo, pairConnect) {
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), 1 TSRMLS_CC);

  zval zusername, zpassword;

  ZVAL_STRING(&zusername, "", 0);
  ZVAL_STRING(&zpassword, "", 0);

  PUSH_PARAM(&zusername); PUSH_PARAM(&zpassword); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_Mongo_connectUtil(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}

/* {{{ Mongo->persistConnect
 */
PHP_METHOD(Mongo, persistConnect) {
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), 1 TSRMLS_CC);

  zval *zusername, *zpassword;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &zusername, &zpassword) == FAILURE) {
    return;
  } 

  PUSH_PARAM(&zusername); PUSH_PARAM(&zpassword); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_Mongo_connectUtil(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}

/* {{{ Mongo->pairPersistConnect
 */
PHP_METHOD(Mongo, pairPersistConnect) {
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), 1 TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), 1 TSRMLS_CC);

  zval *zusername, *zpassword;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &zusername, &zpassword) == FAILURE) {
    return;
  } 

  PUSH_PARAM(&zusername); PUSH_PARAM(&zpassword); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_Mongo_connectUtil(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}


PHP_METHOD(Mongo, connectUtil) {
  // if we're already connected, disconnect
  zval *connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  if (Z_BVAL_P(connected)) {
    // Mongo->close()
    zim_Mongo_close(INTERNAL_FUNCTION_PARAM_PASSTHRU);

    // Mongo->connected = false
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  }
  connect_already(INTERNAL_FUNCTION_PARAM_PASSTHRU, NOT_LAZY);
  
  connected = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"), NOISY TSRMLS_CC);
  // if connecting failed, throw an exception
  if (!Z_BVAL_P(connected)) {
    zval *server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
    zend_throw_exception(mongo_ce_ConnectionException, Z_STRVAL_P(server), 0 TSRMLS_CC);
    return;
  }

  // set the Mongo->connected property
  Z_LVAL_P(connected) = 1;
}


static void connect_already(INTERNAL_FUNCTION_PARAMETERS, int lazy) {
  zval *username, *password;
 
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &username, &password) == FAILURE) {
    return;
  }

  zval *server = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), 0 TSRMLS_CC);

  zval *pair = zend_read_property(mongo_ce_Mongo, getThis(), "paired", strlen("paired"), 0 TSRMLS_CC);
  zval *persist = zend_read_property(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), 0 TSRMLS_CC);

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

  mongo_link *link;
  zend_rsrc_list_entry *le;
  zend_update_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), return_value TSRMLS_CC);

  if (Z_BVAL_P(persist)) {
    char *key;
    int key_len = spprintf(&key, 0, "%s_%s_%s", Z_STRVAL_P(server), Z_STRVAL_P(username), Z_STRVAL_P(password));
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


  link = (mongo_link*)emalloc(sizeof(mongo_link));

  // zero pointers so it doesn't segfault on cleanup if 
  // connection fails
  link->username = 0;
  link->password = 0;
  link->paired = Z_BVAL_P(pair);

  get_host_and_port(Z_STRVAL_P(server), link TSRMLS_CC);
  if (mongo_do_socket_connect(link) == FAILURE) {
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
    int ulen = Z_STRLEN_P(username);
    int plen = Z_STRLEN_P(password);
    if (ulen > 0 && plen > 0) {
      link->username = (char*)emalloc(ulen);
      link->password = (char*)emalloc(plen);
      memcpy(link->username, Z_STRVAL_P(username), ulen);
      memcpy(link->password, Z_STRVAL_P(password), plen);
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

  // extract host:port
  char *colon = strchr(server, ':');
  char *host;
  int port;
  if (colon) {
    int host_len = colon-server+1;
    host = (char*)emalloc(host_len);
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
    char *comma = strchr(server, ',');
    comma++;
    colon = strchr(comma, ':');

    // check that length is sane
    if (colon && 
        colon - comma > 0 &&
        colon - comma < 256) {
      int host_len = colon-comma + 1;
      host = (char*)emalloc(host_len);
      memcpy(host, comma, host_len-1);
      host[host_len-1] = 0;
      port = atoi(colon + 1);
    }
    else {
      host = (char*)emalloc(strlen(comma)+1);
      memcpy(host, comma, strlen(comma));
      host[strlen(comma)] = 0;
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
  zval *db;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &db) == FAILURE) {
    return;
  }

  zval *obj;
  MAKE_STD_ZVAL(obj);
  object_init_ex(obj, mongo_ce_DB);

  PUSH_PARAM(getThis()); PUSH_PARAM(db); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_MongoDB___construct(2, return_value, return_value_ptr, obj, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  RETURN_ZVAL(obj, 0, 1);
}
/* }}} */

/* {{{ Mongo->selectCollection()
 */
PHP_METHOD(Mongo, selectCollection) {
  zval *db, *collection;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &db, &collection) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(db) != IS_OBJECT ||
      Z_OBJCE_P(db) != mongo_ce_DB) {
    zval *temp_db;
    MAKE_STD_ZVAL(temp_db);

    zim_Mongo_selectDB(1, temp_db, &temp_db, getThis(), return_value_used TSRMLS_CC);
    db = temp_db;
  }
  else {
    zval_add_ref(&db);
  }

  PUSH_PARAM(collection); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoDB_selectCollection(1, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&db);
}
/* }}} */

/* {{{ Mongo->dropDB()
 */
PHP_METHOD(Mongo, dropDB) {
  zval *db;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &db) == FAILURE) {
    RETURN_FALSE;
  }

  if (Z_TYPE_P(db) != IS_OBJECT ||
      Z_OBJCE_P(db) != mongo_ce_DB) {
    zval *temp_db;
    MAKE_STD_ZVAL(temp_db);

    zim_Mongo_selectDB(1, temp_db, &temp_db, getThis(), return_value_used TSRMLS_CC);
    db = temp_db;
  }
  else {
    zval_add_ref(&db);
  }

  zim_MongoDB_drop(0, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);

  zval_ptr_dtor(&db);
}
/* }}} */

/* {{{ Mongo->repairDB()
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
  zim_MongoDB_selectCollection(1, return_value, return_value_ptr, db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&backup);
  zval_ptr_dtor(&preserve_clones);
}
/* }}} */

/* {{{ Mongo->lastError()
 */
PHP_METHOD(Mongo, lastError) {
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "getlasterror", 1);

  zval *name;
  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(zlink); PUSH_PARAM(data); PUSH_PARAM(name); PUSH_PARAM((void*)3);
  PUSH_EO_PARAM();
  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);
  zval_ptr_dtor(&data);
}
/* }}} */

/* {{{ Mongo->prevError()
 */
PHP_METHOD(Mongo, prevError) {
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "getpreverror", 1);

  zval *name;
  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(zlink); PUSH_PARAM(data); PUSH_PARAM(name); PUSH_PARAM((void*)3);
  PUSH_EO_PARAM();
  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);
  zval_ptr_dtor(&data);
}
/* }}} */

/* {{{ Mongo->resetError()
 */
PHP_METHOD(Mongo, resetError) {
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), NOISY TSRMLS_CC);

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "reseterror", 1);

  zval *name;
  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(zlink); PUSH_PARAM(data); PUSH_PARAM(name); PUSH_PARAM((void*)3);
  PUSH_EO_PARAM();
  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);
  zval_ptr_dtor(&data);
}
/* }}} */

/* {{{ Mongo->forceError()
 */
PHP_METHOD(Mongo, forceError) {
  mongo_link *link;
  zval *zlink = zend_read_property(mongo_ce_Mongo, getThis(), "connection", strlen("connection"), 0 TSRMLS_CC);
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval *data;
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "forceerror", 1);

  zval *name;
  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);

  PUSH_PARAM(zlink); PUSH_PARAM(data); PUSH_PARAM(name); PUSH_PARAM((void*)3);
  PUSH_EO_PARAM();
  zim_MongoUtil_dbCommand(3, return_value, return_value_ptr, NULL, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&name);
  zval_ptr_dtor(&data);
}
/* }}} */



mongo_cursor* mongo_do_query(mongo_link *link, char *collection, int skip, int limit, zval *zquery, zval *zfields TSRMLS_DC) {
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
  int sent = mongo_say(link, &buf TSRMLS_CC);
  efree(buf.start);
  if (sent == FAILURE) {
    zend_error(E_WARNING, "couldn't send query\n");
    return 0;
  }

  mongo_cursor *cursor = (mongo_cursor*)emalloc(sizeof(mongo_cursor));
  cursor->buf.start = 0;
  cursor->buf.pos = 0;
  cursor->link = link;
  cursor->ns = estrdup(collection);

  get_reply(cursor TSRMLS_CC);

  cursor->limit = limit;
  return cursor;
}


int mongo_do_has_next(mongo_cursor *cursor TSRMLS_DC) {
  if (cursor->num == 0) {
    return 0;
  }
  if (cursor->at < cursor->num) {
    return 1;
  }

  CREATE_BUF(buf, 256);
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
  if (cursor->at >= cursor->num) {
    // check for more results
    if (!mongo_do_has_next(cursor TSRMLS_CC)) {
      // we're out of results
      return 0;
    }
    // we got more results
  }

  if (cursor->at < cursor->num) {
    zval *elem;
    MAKE_STD_ZVAL(elem);
    array_init(elem);
    cursor->buf.pos = bson_to_zval(cursor->buf.pos, elem TSRMLS_CC);

    // increment cursor position
    cursor->at++;
    return elem;
  }
  return 0;
}

int mongo_do_insert(mongo_link *link, char *collection, zval *zarray TSRMLS_DC) {

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, collection, strlen(collection), OP_INSERT);

  // serialize
  if (zval_to_bson(&buf, zarray, PREP TSRMLS_CC) == 0) {
    efree(buf.start);
    // return if there were 0 elements
    return FAILURE;
  }

  serialize_size(buf.start, &buf);

  // sends
  int response = mongo_say(link, &buf TSRMLS_CC);
  efree(buf.start);
  return response;
}

static int get_master(mongo_link *link TSRMLS_DC) {
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
  zval *is_master, *response;
  ALLOC_INIT_ZVAL(is_master);
  array_init(is_master);
  add_assoc_long(is_master, "ismaster", 1);

  mongo_link temp;
  temp.paired = 0;
  temp.server.single.socket = link->server.paired.lsocket;

  // check the left
  mongo_cursor *c = mongo_do_query(&temp, "admin.$cmd", 0, -1, is_master, 0 TSRMLS_CC);

  if ((response = mongo_do_next(c TSRMLS_CC)) != 0) {
    zval **ans;
    if (zend_hash_find(Z_ARRVAL_P(response), "ismaster", 9, (void**)&ans) == SUCCESS && 
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&is_master);
      return link->master = link->server.paired.lsocket;
    }
  }

  // check the right
  temp.server.single.socket = link->server.paired.rsocket;

  c = mongo_do_query(&temp, "admin.$cmd", 0, -1, is_master, 0 TSRMLS_CC);

  if ((response = mongo_do_next(c TSRMLS_CC)) != 0) {
    zval **ans;
    if (zend_hash_find(Z_ARRVAL_P(response), "ismaster", 9, (void**)&ans) == SUCCESS && 
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&is_master);
      return link->master = link->server.paired.rsocket;
    }
  }

  // give up
  zval_ptr_dtor(&is_master);
  return FAILURE;
}


int get_reply(mongo_cursor *cursor TSRMLS_DC) {
  if(cursor->buf.start) {
    efree(cursor->buf.start);
    cursor->buf.start = 0;
  }
  
  // if this fails, we might be disconnected... but we're probably
  // just out of results
  if (mongo_hear(cursor->link, &cursor->header.length, INT_32 TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  // make sure we're not getting crazy data
  if (cursor->header.length > MAX_RESPONSE_LEN ||
      cursor->header.length < REPLY_HEADER_SIZE) {
    zend_error(E_WARNING, "bad response length: %d, did the db assert?\n", cursor->header.length);
    return FAILURE;
  }

  // create buf
  cursor->header.length -= INT_32;
  // point buf.start at buf's first char
  cursor->buf.start = (unsigned char*)emalloc(cursor->header.length);
  cursor->buf.pos = cursor->buf.start;
  cursor->buf.end = cursor->buf.start + cursor->header.length;

  if (mongo_hear(cursor->link, cursor->buf.pos, cursor->header.length TSRMLS_CC) == FAILURE) {
    zend_error(E_WARNING, "error getting response buf: %s\n", strerror(errno));
    return FAILURE;
  }

  memcpy(&cursor->header.request_id, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  memcpy(&cursor->header.response_to, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  memcpy(&cursor->header.op, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;

  memcpy(&cursor->flag, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  if(cursor->flag == CURSOR_NOT_FOUND) {
    cursor->num = 0;
    return FAILURE;
  }

  memcpy(&cursor->cursor_id, cursor->buf.pos, INT_64);
  cursor->buf.pos += INT_64;
  memcpy(&cursor->start, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  memcpy(&cursor->num, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;

  cursor->at = 0;

  return cursor->num == 0 ? FAILURE : SUCCESS;
}


int mongo_say(mongo_link *link, buffer *buf TSRMLS_DC) {
  if (check_connection(link TSRMLS_CC) == SUCCESS) {

#ifdef WIN32
    int sent = send(get_master(link TSRMLS_CC), (const char*)buf->start, buf->pos-buf->start, FLAGS);
#else
	int sent = send(get_master(link TSRMLS_CC), buf->start, buf->pos-buf->start, FLAGS);
#endif

    if (sent == FAILURE) {
      link->server.single.connected = 0;
      sent = check_connection(link TSRMLS_CC);
    }
    return sent;
  }
  return FAILURE;
}

int mongo_hear(mongo_link *link, void *dest, int len TSRMLS_DC) {
  int tries = 3, num = 1;
  while (check_connection(link TSRMLS_CC) == FAILURE &&
         tries > 0) {
#ifndef WIN32
    sleep(2);
#endif
    zend_error(E_WARNING, "no connection, trying to reconnect %d more times...\n", tries--);
  }
  // this can return FAILED if there is just no more data from db
  int r = 0;
  while(r < len && num > 0) {

#ifdef WIN32
    num = recv(get_master(link TSRMLS_CC), (char*)dest, len, FLAGS);
#else
    num = recv(get_master(link TSRMLS_CC), dest, len, FLAGS);
#endif

	dest = (char*)dest + num;
    r += num;
  }
  return r;
}

static int check_connection(mongo_link *link TSRMLS_DC) {
  if (!MonGlo(auto_reconnect) ||
      (!link->paired && link->server.single.connected) || 
      (link->paired && 
       (link->server.paired.lconnected || 
        link->server.paired.rconnected)) ||
      time(0)-link->ts < 2) {
    return SUCCESS;
  }

  link->ts = time(0);

  if (link->paired) {
    close(link->server.paired.lsocket);
    close(link->server.paired.rsocket);
  }
  else {
    close(link->server.single.socket);
  } 

  return mongo_do_socket_connect(link);
}

static int mongo_connect_nonb(int sock, char *host, int port) {
  struct sockaddr_in addr, addr2;
  fd_set rset, wset;
  struct timeval tval;
#ifdef WIN32
  const char yes = 1;
#else
  int yes = 1;
#endif

  int connected = FAILURE;

  // timeout
  tval.tv_sec = 20;
  tval.tv_usec = 0;

  // get addresses
  get_sockaddr(&addr, host, port);

  // create sockets
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == FAILURE) {
    zend_error (E_WARNING, "couldn't create socket");
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
    if (errno != WSAEINPROGRESS) {
#else
    if (errno != EINPROGRESS) {
#endif
      close(sock);
      zend_error(E_WARNING, "%s:%d: %s", host, port, strerror(errno));
      return FAILURE;
    }

    if (select(sock+1, &rset, &wset, NULL, &tval) == 0) {
      close(sock);
      return FAILURE;
    }

    uint size = sizeof(addr2);
    connected = getpeername(sock, (struct sockaddr*)&addr, &size);
    if (connected == FAILURE) {
      close(sock);
      return FAILURE;
    }
  }

  // reset flags
#ifdef WIN32
  u_long no = 0;
  ioctlsocket(sock, FIONBIO, &no);
#else
  fcntl(sock, F_SETFL, FLAGS);
#endif

  return sock;
}

static int mongo_do_socket_connect(mongo_link *link) {
  if (link->paired) {

    if ((link->server.paired.lsocket =
         mongo_connect_nonb(link->server.paired.lsocket,
                            link->server.paired.left,
                            link->server.paired.lport)) == FAILURE) {

      link->server.paired.lconnected = 0;
    }
    else {
      link->server.paired.lconnected = 1;
    }
    if ((link->server.paired.rsocket =
         mongo_connect_nonb(link->server.paired.lsocket,
                            link->server.paired.left,
                            link->server.paired.lport)) == FAILURE) {
      link->server.paired.rconnected = 0;
    }
    else {
      link->server.paired.rconnected = 1;
    }

    if (!link->server.paired.lconnected &&
        !link->server.paired.rconnected) {
      return FAILURE;
    }
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

static int get_sockaddr(struct sockaddr_in *addr, char *host, int port) {
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  struct hostent *hostinfo = (struct hostent*)gethostbyname(host);
  if (hostinfo == NULL) {
    zend_error (E_WARNING, "unknown host %s", host);
    efree(host);
    return FAILURE;
  }
  addr->sin_addr = *((struct in_addr*)hostinfo->h_addr);
  return SUCCESS;
}

