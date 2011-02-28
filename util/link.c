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

#include "php_mongo.h"
#include "link.h"
#include "rs.h"

extern zend_class_entry *mongo_ce_Mongo;
ZEND_EXTERN_MODULE_GLOBALS(mongo);


/** Attempts to find a slave to read from.
 * Returns 0 and sets errmsg on failure.
 */
mongo_server* mongo_util_link_get_slave_socket(mongo_link *link, zval *errmsg TSRMLS_DC) {
  int now, status;
  
  // sanity check
  if (!link->rs) {
    ZVAL_STRING(errmsg, "Connection is not a replica set", 1);
    return 0;
  }

  // every 5 seconds, try to update hosts
  now = time(0);
  if (link->server_set && link->server_set->ts + 5 < now) {
    zval *fake_zval;
    mongo_link *fake_link;
    
    link->server_set->ts = now;
    
    MAKE_STD_ZVAL(fake_zval);
    object_init_ex(fake_zval, mongo_ce_Mongo);
    fake_link = (mongo_link*)zend_object_store_get_object(fake_zval TSRMLS_CC);
    fake_link->server_set = link->server_set;

    get_heartbeats(fake_zval, &(Z_STRVAL_P(errmsg)) TSRMLS_CC);
    
    // if get_heartbeats fails, ignore
    fake_link->server_set = 0;
    zval_ptr_dtor(&fake_zval);
  }

  if (link->slave) {
    zval *fake_zval;
    mongo_link *fake_link;
    mongo_server_set fake_set;
    mongo_server *temp;

    if (link->slave->connected) {
      return link->slave;
    }

    MAKE_STD_ZVAL(fake_zval);
    object_init_ex(fake_zval, mongo_ce_Mongo);
    fake_link = (mongo_link*)zend_object_store_get_object(fake_zval TSRMLS_CC);
    fake_link->server_set = &fake_set;
    fake_link->rs = 0;
    
    temp = link->slave->next;
    link->slave->next = 0;
    fake_set.server = link->slave;
    fake_set.master = link->slave;
    fake_set.num = 1;
    fake_set.ts = time(0);
    
    if (php_mongo_do_socket_connect(fake_link, errmsg TSRMLS_CC) == SUCCESS) {
      link->slave->next = temp;
      fake_link->server_set = 0;
      zval_ptr_dtor(&fake_zval);
      return link->slave;
    }
    link->slave->next = temp; 
    fake_link->server_set = 0;
    zval_ptr_dtor(&fake_zval);

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
  int now = time(0), connected = 0;
  
  if ((link->server_set->num == 1 && !link->rs && link->server_set->server->connected) ||
      (link->server_set->master && link->server_set->master->connected)) {
    connected = 1;
  }

  // if we're already connected or autoreconnect isn't set, we're all done 
  if (!MonGlo(auto_reconnect) || connected) {
    mongo_server *server = mongo_util_rs_get_master(link TSRMLS_CC);
    if (!server) {
      ZVAL_STRING(errmsg, "Couldn't determine master", 1);
    }
    return server;
  }

  // close connection
  mongo_util_link_disconnect(link);

  if (SUCCESS == php_mongo_do_socket_connect(link, errmsg TSRMLS_CC)) {
    mongo_server *server = mongo_util_rs_get_master(link TSRMLS_CC);
    if (!server) {
      ZVAL_STRING(errmsg, "Couldn't determine master", 1);
    }
    return server;
  }

  // errmsg set in do_socket_connect
  return 0;
}


// TODO: this should disconnect from all members of the RS
void mongo_util_link_disconnect(mongo_link *link) {
  // already disconnected
  if (!link->server_set->master ||
      !link->server_set->master->connected) {
    return;
  }

  // sever it
  php_mongo_disconnect_server(link->server_set->master);
  link->server_set->master = 0;
}

