// mongo.c
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


#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef WIN32
#include <sys/types.h>
#endif

#include <php.h>
#include <zend_exceptions.h>

#include "php_mongo.h"
#include "mongo.h"
#include "db.h"
#include "cursor.h"
#include "mongo_types.h"
#include "bson.h"

#include "util/rs.h"
#include "util/parse.h"
#include "util/io.h"
#include "util/hash.h"
#include "util/connect.h"
#include "util/pool.h"
#include "util/link.h"
#include "util/server.h"
#include "util/log.h"

static void php_mongo_link_free(void* TSRMLS_DC);
static void run_err(int, zval*, zval* TSRMLS_DC);
static char* stringify_server(mongo_server*, char*, int*, int*);

zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_Mongo;

extern zend_class_entry *mongo_ce_DB,
  *mongo_ce_Cursor,
  *mongo_ce_Exception,
  *mongo_ce_ConnectionException;

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


static zend_function_entry mongo_methods[] = {
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
  PHP_ME(Mongo, setPoolSize, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Mongo, getPoolSize, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Mongo, poolDebug, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Mongo, serverInfo, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  { NULL, NULL, NULL }
};


void php_mongo_server_free(mongo_server *server, int persist TSRMLS_DC) {
  // return this connection to the pool
  mongo_util_pool_done(server TSRMLS_CC);

  if (server->host) {
    pefree(server->host, persist);
    server->host = 0;
  }
  if (server->label) {
    pefree(server->label, persist);
    server->label = 0;
  }
  if (server->username) {
    pefree(server->username, persist);
    server->username = 0;
  }
  if (server->password) {
    pefree(server->password, persist);
    server->password = 0;
  }
  if (server->db) {
    pefree(server->db, persist);
    server->db = 0;
  }

  pefree(server, persist);
}

static void php_mongo_server_set_free(mongo_server_set *server_set TSRMLS_DC) {
  mongo_server *current;

  if (!server_set) {
    return;
  }

  current = server_set->server;

  while (current) {
    mongo_server *temp = current->next;
    php_mongo_server_free(current, NO_PERSIST TSRMLS_CC);
    current = temp;
  }

  efree(server_set);
}

/* {{{ php_mongo_link_free
 */
static void php_mongo_link_free(void *object TSRMLS_DC) {
  mongo_link *link = (mongo_link*)object;

  // already freed
  if (!link) {
    return;
  }

  if (link->rs) {
    mongo_server *current = link->server_set->server;

    if (link->server_set->master) {
      php_mongo_server_free(link->server_set->master, NO_PERSIST TSRMLS_CC);
    }
    if (link->slave) {
      php_mongo_server_free(link->slave, NO_PERSIST TSRMLS_CC);
    }
  }

  php_mongo_server_set_free(link->server_set TSRMLS_CC);

  if (link->username) efree(link->username);
  if (link->password) efree(link->password);
  if (link->db) efree(link->db);
  if (link->rs) efree(link->rs);

  zend_object_std_dtor(&link->std TSRMLS_CC);

  efree(link);
}
/* }}} */


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
        if (Z_TYPE_PP(replica_z) == IS_STRING) {
          link->rs = estrdup(Z_STRVAL_PP(replica_z));
        }
        else if (Z_BVAL_PP(replica_z)) {
          link->rs = estrdup("replicaSet");
        }
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
  char *msg = 0;
  zval *connected_z = 0;

  connected_z = zend_read_property(mongo_ce_Mongo, getThis(), "connected", strlen("connected"),
                                   QUIET TSRMLS_CC);
  if (Z_TYPE_P(connected_z) == IS_BOOL && Z_BVAL_P(connected_z)) {
    RETURN_TRUE;
  }

  link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (link->rs) {
    // connected will be 1 unless something goes very wrong. we might not
    // actually be connected
    connected = (mongo_util_rs_init(link TSRMLS_CC) == SUCCESS);

    if (!connected && !EG(exception)) {
      msg = estrdup("Could not create replica set connection");
    }
  }
  else {
    mongo_server *current;
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
  }

  if (!connected) {
    zend_throw_exception(mongo_ce_ConnectionException, msg, 0 TSRMLS_CC);
  }
  else {
    zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected",
                              strlen("connected"), 1 TSRMLS_CC);
    ZVAL_BOOL(return_value, 1);
  }

  if (msg) {
    efree(msg);
  }
}

/* {{{ Mongo->close()
 */
PHP_METHOD(Mongo, close) {
  mongo_link *link;

  PHP_MONGO_GET_LINK(getThis());

  mongo_util_link_disconnect(link TSRMLS_CC);

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
    ZVAL_NULL(temp_db);

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

PHP_METHOD(Mongo, getHosts)
{
	mongo_link  *link;
	rsm_server  *current;
	rs_monitor  *monitor;

	array_init(return_value);
	PHP_MONGO_GET_LINK(getThis());

	monitor = mongo_util_rs__get_monitor(link TSRMLS_CC);

	current = monitor->servers;
	while (current) {
		zval *infoz;
		server_info *info;

		MAKE_STD_ZVAL(infoz);
		array_init(infoz);

		info = mongo_util_server__get_info(current->server TSRMLS_CC);
		add_assoc_long(infoz, "health", info->guts->readable);
		add_assoc_long(infoz, "state", info->guts->master ? 1 : info->guts->readable ? 2 : 0);
		if (info->guts->pinged) {
			add_assoc_long(infoz, "ping", info->guts->ping);
			add_assoc_long(infoz, "lastPing", info->guts->last_ping);
		}

		add_assoc_zval(return_value, current->server->label, infoz);
		current = current->next;
	}
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

  mongo_util_rs_ping(link TSRMLS_CC);
  if (mongo_util_rs__set_slave(link, &errmsg TSRMLS_CC) == FAILURE) {
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

