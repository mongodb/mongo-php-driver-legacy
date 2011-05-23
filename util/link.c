// link.c
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

#include "../php_mongo.h"
#include "link.h"
#include "rs.h"
#include "pool.h"
#include "connect.h"

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_ConnectionException;
ZEND_EXTERN_MODULE_GLOBALS(mongo);


mongo_server* mongo_util_link_get_slave_socket(mongo_link *link, zval *errmsg TSRMLS_DC) {
  int status;
  
  // sanity check
  if (!link->rs) {
    ZVAL_STRING(errmsg, "Connection is not a replica set", 1);
    return 0;
  }
  
  // see if we need to update hosts or ping them
  mongo_util_rs_ping(link TSRMLS_CC);

  if (link->slave) {
    if (link->slave->connected) {
      return link->slave;
    }

    if (mongo_util_pool_get(link->slave, errmsg TSRMLS_CC) == SUCCESS) {
      return link->slave;
    }

    // TODO: what if we can't reconnect?  close cursors? grab another slave?
  }
  
  status = set_a_slave(link, &(Z_STRVAL_P(errmsg)));
  if (status == FAILURE) {
    ZVAL_STRING(errmsg, "Could not find any server to read from", 1);
    return 0;
  }

  return link->slave;
}



/*
 * If the socket is connected, returns the master.  If the socket is
 * disconnected, it attempts to reconnect and return the master.
 *
 * sets errmsg and returns 0 on failure
 */
mongo_server* mongo_util_link_get_socket(mongo_link *link, zval *errmsg TSRMLS_DC) {
  int connected = 0;
  
  if ((link->server_set->num == 1 && !link->rs && link->server_set->server->connected) ||
      (link->server_set->master && link->server_set->master->connected)) {
    connected = 1;
  }

  // if we're already connected or autoreconnect isn't set, we're all done 
  if (!MonGlo(auto_reconnect) || connected) {
    mongo_server *server = mongo_util_rs_get_master(link TSRMLS_CC);
    if (!server) {
      ZVAL_STRING(errmsg, "Couldn't determine master", 1);
      link->server_set->master = 0;
    }
    return server;
  }

  // close connection
  mongo_util_link_disconnect(link);

  if (SUCCESS == mongo_util_link_try_connecting(link, errmsg TSRMLS_CC)) {
    mongo_server *server = mongo_util_rs_get_master(link TSRMLS_CC);
    if (!server) {
      ZVAL_STRING(errmsg, "Couldn't determine master", 1);
      link->server_set->master = 0;
    }
    return server;
  }

  // errmsg set in try_connecting
  return 0;
}


int mongo_util_link_try_connecting(mongo_link *link, zval *errmsg TSRMLS_DC) {
  zval *errmsg_holder = 0;
  mongo_server *current = 0;
  int connected = 0;
  
  // initialize and clear the error message
  MAKE_STD_ZVAL(errmsg_holder);
  ZVAL_NULL(errmsg_holder);

  current = link->server_set->server;
  connected = 0;
  
  log0("connecting");
  
  while (current) {
    connected |= (mongo_util_pool_get(current, errmsg TSRMLS_CC) == SUCCESS);
    
    log3("%s:%d connected? %s\n", current->host, current->port, connected == 0 ? "true" : "false");

    if (Z_TYPE_P(errmsg_holder) == IS_STRING) {
      if (Z_TYPE_P(errmsg) == IS_NULL) {
        ZVAL_STRING(errmsg, Z_STRVAL_P(errmsg_holder), 1);
      }
      zval_ptr_dtor(&errmsg_holder);
      MAKE_STD_ZVAL(errmsg_holder);
      ZVAL_NULL(errmsg_holder);
    }
    
    current = current->next;
  }

  zval_ptr_dtor(&errmsg_holder);
  
  if (!connected) {
    if (Z_TYPE_P(errmsg) != IS_STRING) {
      // there should always be an error message, we should never get here
      ZVAL_STRING(errmsg, "connecting failed", 1);
    }

    return FAILURE;
  }

  /*
   * cases where we have an error message and don't care because there's a
   * connection we can use:
   *  - if a connections fails after we have at least one working
   *  - if the first connection fails but a subsequent ones succeeds
   */
  if (Z_TYPE_P(errmsg) == IS_STRING) {
    efree(Z_STRVAL_P(errmsg));
    ZVAL_NULL(errmsg);
  }
  return SUCCESS;
}


void mongo_util_link_disconnect(mongo_link *link) {
  mongo_server *current = link->server_set->server;

  while (current) {
    mongo_util_disconnect(current);
    current = current->next;
  }
  
  link->server_set->master = 0;
}

