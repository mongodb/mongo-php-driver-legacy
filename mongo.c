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
#include "ext/standard/php_smart_str.h"

#include "php_mongo.h"
#include "mongo.h"
#include "db.h"
#include "cursor.h"
#include "mongo_types.h"
#include "bson.h"

#include "util/rs.h"
#include "util/parse.h"
#include "util/hash.h"
#include "util/connect.h"
#include "util/pool.h"
#include "util/link.h"
#include "util/server.h"
#include "util/log.h"

#include "mcon/types.h"
#include "mcon/read_preference.h"
#include "mcon/parse.h"
#include "mcon/manager.h"
#include "mcon/utils.h"


static void php_mongo_link_free(void* TSRMLS_DC);
static void run_err(int, zval*, zval* TSRMLS_DC);
static void stringify_server(mongo_server_def *server, smart_str *str);

zend_object_handlers mongo_default_handlers;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

extern mongo_con_manager *manager;

zend_class_entry *mongo_ce_Mongo;

extern zend_class_entry *mongo_ce_DB,
  *mongo_ce_Cursor,
  *mongo_ce_Exception,
  *mongo_ce_ConnectionException;

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, server)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
/* Those two used to be there, but no longer it seems
	ZEND_ARG_INFO(0, persist)
	ZEND_ARG_INFO(0, garbage)
*/
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_selectDB, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, database_name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_selectCollection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, database_name)
	ZEND_ARG_INFO(0, collection_name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_setSlaveOkay, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, slave_okay)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_dropDB, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, MongoDB_object_OR_database_name)
ZEND_END_ARG_INFO()

MONGO_ARGINFO_STATIC ZEND_BEGIN_ARG_INFO_EX(arginfo_setPoolSize, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, maximum_pool_size)
ZEND_END_ARG_INFO()

static zend_function_entry mongo_methods[] = {
  PHP_ME(Mongo, __construct, arginfo___construct, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, connect, arginfo_no_parameters, ZEND_ACC_PUBLIC)
//  PHP_ME(Mongo, pairConnect, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
//  PHP_ME(Mongo, persistConnect, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
//  PHP_ME(Mongo, pairPersistConnect, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, connectUtil, arginfo_no_parameters, ZEND_ACC_PROTECTED)
  PHP_ME(Mongo, __toString, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, __get, arginfo___get, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, selectDB, arginfo_selectDB, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, selectCollection, arginfo_selectCollection, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, getSlaveOkay, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, setSlaveOkay, arginfo_setSlaveOkay, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, dropDB, arginfo_dropDB, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, lastError, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, prevError, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, resetError, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, forceError, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, listDBs, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, getHosts, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(Mongo, getSlave, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, switchSlave, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
  PHP_ME(Mongo, close, arginfo_no_parameters, ZEND_ACC_PUBLIC)
//  PHP_ME(Mongo, setPoolSize, arginfo_setPoolSize, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC|ZEND_ACC_DEPRECATED)
//  PHP_ME(Mongo, getPoolSize, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC|ZEND_ACC_DEPRECATED)
//  PHP_ME(Mongo, poolDebug, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
//  PHP_ME(Mongo, serverInfo, arginfo_no_parameters, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  { NULL, NULL, NULL }
};

/* {{{ php_mongo_link_free
 */
static void php_mongo_link_free(void *object TSRMLS_DC)
{
	mongo_link *link = (mongo_link*)object;

	/* already freed */
	if (!link) {
		return;
	}

	if (link->servers) {
		mongo_servers_dtor(link->servers);
	}

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
PHP_METHOD(Mongo, __construct)
{
  char *server = 0;
  int server_len = 0;
  zend_bool persist = 0, garbage = 0, connect = 1;
  zval *options = 0, *slave_okay = 0;
  mongo_link *link;
	int i;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|szbb", &server, &server_len, &options, &persist, &garbage) == FAILURE) {
    zval *object = getThis();
    ZVAL_NULL(object);
    return;
  }
  if (garbage) {
    php_error_docref(NULL TSRMLS_CC, MONGO_E_DEPRECATED, "This argument doesn't actually do anything. Please stop using it");
  }

	link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

	/* Set the manager from the global manager */
	link->manager = manager;
	
	/* Parse the server specification */
	link->servers = mongo_parse_server_spec(server ? server : "mongodb://127.0.0.1");

	slave_okay = zend_read_static_property(mongo_ce_Cursor, "slaveOkay", strlen("slaveOkay"), NOISY TSRMLS_CC);
	if (Z_BVAL_P(slave_okay)) {
		if (link->servers->rp.type != MONGO_RP_PRIMARY) {
			/* the server already has read preferences configured, but we're still
			 * trying to set slave okay. The spec says that's an error */
			zend_throw_exception(mongo_ce_ConnectionException, "You can not use both slaveOkay and read-preferences. Please switch to read-preferences.", 0 TSRMLS_CC);
		} else {
			/* Old style option, that needs to be removed. For now, spec dictates
			 * it needs to be ReadPreference=SECONDARY_PREFERRED */
			link->servers->rp.type = MONGO_RP_SECONDARY_PREFERRED;
		}
	}

	/* Options through array */
	if (options) {
		if (Z_TYPE_P(options) == IS_ARRAY) {
			zval **timeout_z, **replica_z, **slave_okay_z, **username_z, **password_z,
			**db_z, **connect_z;

			if (zend_hash_find(HASH_P(options), "timeout", strlen("timeout")+1, (void**)&timeout_z) == SUCCESS) {
				link->servers->connectTimeoutMS = Z_LVAL_PP(timeout_z);
			}
			if (zend_hash_find(HASH_P(options), "replicaSet", strlen("replicaSet")+1, (void**)&replica_z) == SUCCESS) {
				/* Setting the replica set name automatically triggers
				 * the connection type to be set as REPLSET */
				link->servers->con_type = MONGO_CON_TYPE_REPLSET;

				if (link->servers->repl_set_name) {
					/* Free the already existing one */
					free(link->servers->repl_set_name);
					link->servers->repl_set_name = NULL; /* We reset it as not all options set a string as replset name */
				}
				if (Z_TYPE_PP(replica_z) == IS_STRING) {
					link->servers->repl_set_name = strdup(Z_STRVAL_PP(replica_z));
				} else if ((Z_TYPE_PP(replica_z) == IS_BOOL || Z_TYPE_PP(replica_z) == IS_LONG) && !Z_BVAL_PP(replica_z)) {
					/* Turn off replica set handling, which means either use a
					 * standalone server, or a "multi-set". Why you would do
					 * this? No idea. */
					if (link->servers->count == 1) {
						link->servers->con_type = MONGO_CON_TYPE_STANDALONE;
					} else {
						link->servers->con_type = MONGO_CON_TYPE_MULTIPLE;
					}
				}
			}

			if (zend_hash_find(HASH_P(options), "slaveOkay", strlen("slaveOkay")+1, (void**)&slave_okay_z) == SUCCESS) {
				if (Z_BVAL_PP(slave_okay_z)) {
					if (link->servers->rp.type != MONGO_RP_PRIMARY) {
						/* the server already has read preferences configured, but we're still
						 * trying to set slave okay. The spec says that's an error */
						zend_throw_exception(mongo_ce_ConnectionException, "You can not use both slaveOkay and read-preferences. Please switch to read-preferences.", 0 TSRMLS_CC);
					} else {
						/* Old style option, that needs to be removed. For now, spec dictates
						 * it needs to be ReadPreference=SECONDARY_PREFERRED */
						link->servers->rp.type = MONGO_RP_SECONDARY_PREFERRED;
					}
				}
			}
			if (zend_hash_find(HASH_P(options), "username", sizeof("username"), (void**)&username_z) == SUCCESS) {
				/* Update all servers in the set */
				for (i = 0; i < link->servers->count; i++) {
					if (link->servers->server[i]->username) {
						free(link->servers->server[i]->username);
					}
					link->servers->server[i]->username = strdup(Z_STRVAL_PP(username_z));
				}
			}
			if (zend_hash_find(HASH_P(options), "password", sizeof("password"), (void**)&password_z) == SUCCESS) {
				/* Update all servers in the set */
				for (i = 0; i < link->servers->count; i++) {
					if (link->servers->server[i]->password) {
						free(link->servers->server[i]->password);
					}
					link->servers->server[i]->password = strdup(Z_STRVAL_PP(password_z));
				}
			}
			if (zend_hash_find(HASH_P(options), "db", sizeof("db"), (void**)&db_z) == SUCCESS) {
				/* Update all servers in the set */
				for (i = 0; i < link->servers->count; i++) {
					if (link->servers->server[i]->db) {
						free(link->servers->server[i]->db);
					}
					link->servers->server[i]->db = strdup(Z_STRVAL_PP(db_z));
				}
			}
			if (zend_hash_find(HASH_P(options), "connect", sizeof("connect"), (void**)&connect_z) == SUCCESS) {
				connect = Z_BVAL_PP(connect_z);
			}
		} else {
			// backwards compatibility
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "Passing scalar values for the options parameter is deprecated and will be removed in the near future");
			connect = Z_BVAL_P(options);
			if (MonGlo(allow_persistent) && persist) {
				zend_update_property_string(mongo_ce_Mongo, getThis(), "persistent", strlen("persistent"), "" TSRMLS_CC);
			}
		}
	}

	if (connect) {
		MONGO_METHOD(Mongo, connectUtil, NULL, getThis());
	}
}
/* }}} */

/* {{{ Helper for connecting the servers */
static void php_mongo_connect(mongo_link *link TSRMLS_DC)
{
	mongo_connection *con;
	char *error_message = NULL;

	/* We don't care about the result so we're not assigning it to a var */
	/* TODO: Implement error messages forwarding to exceptions */
	con = mongo_get_connection(link->manager, link->servers, (char **) &error_message);
	if (!con && error_message) {
		zend_throw_exception(mongo_ce_ConnectionException, error_message, 71 TSRMLS_CC);
		return;
	}
}

/* {{{ Mongo->connect
 */
PHP_METHOD(Mongo, connect)
{
	mongo_link *link;

	PHP_MONGO_GET_LINK(getThis());
	php_mongo_connect(link TSRMLS_CC);
}
/* }}} */

/* {{{ Mongo->connectUtil
 */
PHP_METHOD(Mongo, connectUtil)
{
	mongo_link *link;

	PHP_MONGO_GET_LINK(getThis());
	php_mongo_connect(link TSRMLS_CC);
}
/* }}} */


/* {{{ Mongo->close()
 */
PHP_METHOD(Mongo, close) {
/* TODO: IMPLEMENT
  mongo_link *link;

  PHP_MONGO_GET_LINK(getThis());

  mongo_util_link_disconnect(link TSRMLS_CC);

  zend_update_property_bool(mongo_ce_Mongo, getThis(), "connected",
                            strlen("connected"), 0 TSRMLS_CC);
*/
  RETURN_TRUE;
}
/* }}} */

static void stringify_server(mongo_server_def *server, smart_str *str)
{
	/* if this host is not connected, enclose in []s: [localhost:27017]
	if (!server->connected) {
	str[*pos] = '[';
	*(pos) = *pos + 1;
	}
	*/

	/* copy host */
	smart_str_appends(str, server->host);
	smart_str_appendc(str, ':');
	smart_str_append_long(str, server->port);

	/* close []s
	if (!server->connected) {
	str[*pos] = ']';
	*(pos) = *pos + 1;
	}
	*/
}


/* {{{ Mongo->__toString()
 */
PHP_METHOD(Mongo, __toString) {
	smart_str str = { 0 };
	mongo_link *link;
	int i;

	PHP_MONGO_GET_LINK(getThis());

	for (i = 0; i < link->servers->count; i++) {
		/* if this is not the first one, add a comma */
		if (i) {
			smart_str_appendc(&str, ',');
		}

		stringify_server(link->servers->server[i], &str);
	}

	smart_str_0(&str);

	RETURN_STRING(str.c, 0);
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

PHP_METHOD(Mongo, getSlaveOkay)
{
	mongo_link *link;
	PHP_MONGO_GET_LINK(getThis());
	RETURN_BOOL(link->servers->rp.type != MONGO_RP_PRIMARY);
}

PHP_METHOD(Mongo, setSlaveOkay)
{
	zend_bool slave_okay = 1;
	mongo_link *link;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &slave_okay) == FAILURE) {
		return;
	}

	PHP_MONGO_GET_LINK(getThis());

	RETVAL_BOOL(link->servers->rp.type != MONGO_RP_PRIMARY);
	link->servers->rp.type = slave_okay ? MONGO_RP_SECONDARY_PREFERRED : MONGO_RP_PRIMARY;
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
	mongo_link             *link;
	mongo_con_manager_item *item;

	PHP_MONGO_GET_LINK(getThis());
	item = link->manager->connections;

	array_init(return_value);

	while (item) {
		zval *infoz;
		char *host;
		int   port;

		MAKE_STD_ZVAL(infoz);
		array_init(infoz);

		mongo_server_split_hash(item->connection->hash, (char**) &host, (int*) &port, NULL, NULL, NULL);
		add_assoc_string(infoz, "host", host, 1);
		add_assoc_long(infoz, "port", port);
		free(host);

		add_assoc_long(infoz, "health", 1);
		add_assoc_long(infoz, "state", item->connection->connection_type == MONGO_NODE_PRIMARY ? 1 : (item->connection->connection_type == MONGO_NODE_SECONDARY ? 2 : 0));
		add_assoc_long(infoz, "ping", item->connection->ping_ms);
		add_assoc_long(infoz, "lastPing", item->connection->last_ping);

		add_assoc_zval(return_value, item->connection->hash, infoz);
		item = item->next;
	}
}

PHP_METHOD(Mongo, getSlave)
{
	mongo_link *link;
	mongo_connection *con;
	char *error_message = NULL;

	PHP_MONGO_GET_LINK(getThis());
	con = mongo_get_connection(link->manager, link->servers, (char**) &error_message);
	if (!con && error_message) {
		zend_throw_exception(mongo_ce_ConnectionException, error_message, 71 TSRMLS_CC);
		return;
	}

	RETURN_STRING(con->hash, 1);
}

PHP_METHOD(Mongo, switchSlave)
{
	zim_Mongo_switchSlave(INTERNAL_FUNCTION_PARAM_PASSTHRU);
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

