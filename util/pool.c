// pool.c
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
#include "hash.h"
#include "pool.h"
#include "connect.h"
#include "server.h"

ZEND_EXTERN_MODULE_GLOBALS(mongo);

extern int le_pconnection;

int mongo_util_pool_init(mongo_server *server, time_t timeout TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }
  if (timeout) {
    monitor->timeout = timeout;
  }

  // TODO: do something to num.remaining

  // TODO: initialize pool to a certain size?
  return SUCCESS;
}

int mongo_util_pool_get(mongo_server *server, zval *errmsg TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  // get connection from pool or create new
  if (mongo_util_pool__stack_pop(monitor, server) == SUCCESS ||
      mongo_util_pool__connect(monitor, server, errmsg TSRMLS_CC) == SUCCESS) {
    mongo_util_pool__add_server_ptr(monitor, server);
    return SUCCESS;
  }

  return FAILURE;
}

void mongo_util_pool_done(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    // if we couldn't push this, close the connection
    mongo_util_disconnect(server);
    return;
  }

  // clean up reference to server (nothing needs to be freed)
  mongo_util_pool__rm_server_ptr(monitor, server);

  // if this is disconnected, do not add it to the pool
  if (server->connected) {
    mongo_util_pool__stack_push(monitor, server);
  }
}

void mongo_util_pool_remove(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    // if we couldn't push this, close the connection
    mongo_util_disconnect(server);
    return;
  }

  // clean up reference to server (nothing needs to be freed)
  mongo_util_pool__rm_server_ptr(monitor, server);
}

void mongo_util_pool_close(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    mongo_util_disconnect(server);
    return;
  }

  mongo_util_pool__disconnect(monitor, server);

  // clean up reference to server (nothing needs to be freed)
  mongo_util_pool__rm_server_ptr(monitor, server);
}

int mongo_util_pool_failed(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;
  zval *errmsg;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    mongo_util_disconnect(server);
    return FAILURE;
  }

  mongo_util_pool__close_connections(monitor);
  // just to be sure
  mongo_util_pool__disconnect(monitor, server);

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  // if we cannot reconnect, we'll assume that this server is down
  if (mongo_util_pool__connect(monitor, server, errmsg TSRMLS_CC) == FAILURE) {
    mongo_util_server_down(server TSRMLS_CC);
    zval_ptr_dtor(&errmsg);
    return FAILURE;
  }

  zval_ptr_dtor(&errmsg);
  return SUCCESS;
}

void mongo_util_pool_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  stack_monitor *monitor = 0;

  if (!rsrc || !rsrc->ptr) {
    return;
  }

  monitor = (stack_monitor*)rsrc->ptr;
  mongo_util_pool__close_connections(monitor);
  free(monitor);
  rsrc->ptr = 0;
}

int mongo_util_pool__stack_pop(stack_monitor *monitor, mongo_server *server) {
  stack_node *node;

  node = monitor->top;

  // check that monitor->pop != NULL and pop stack
  if (!node) {
    server->connected = 0;
    return FAILURE;
  }

  monitor->top = node->next;
  monitor->num.in_pool--;

  // theoretically, all servers in the pool should be connected
  server->connected = 1;
  server->socket = node->socket;

  free(node);

  return SUCCESS;
}

void mongo_util_pool__stack_push(stack_monitor *monitor, mongo_server *server) {
  stack_node *node;

  if (server->connected) {
    node = (stack_node*)malloc(sizeof(stack_node));

    node->socket = server->socket;
    node->next = monitor->top;
    monitor->top = node;

    monitor->num.in_pool++;
    server->connected = 0;
  }

  // don't keep more than 50 connections around
  node = monitor->top;
  if (monitor->num.in_pool > 50) {
    int count = 0;
    stack_node *next;

    while (node && count < 50) {
      node = node->next;
      count++;
    }

    next = node->next;
    node->next = 0;
    node = next;

    // get rid of old connections
    while (node) {
      next = node->next;

      MONGO_UTIL_DISCONNECT(node->socket);
      monitor->num.remaining++;
      free(node);

      node = next;
      monitor->num.in_pool--;
    }
  }
}

void mongo_util_pool__stack_clear(stack_monitor *monitor) {
  // holder for popping sockets
  mongo_server temp;

  while (mongo_util_pool__stack_pop(monitor, &temp) == SUCCESS) {
    mongo_util_pool__disconnect(monitor, &temp);
  }
  monitor->top = 0;
}


void mongo_util_pool__add_server_ptr(stack_monitor *monitor, mongo_server *server) {
  mongo_server *list, *current;

  current = monitor->servers;
  while (current) {
    // we are reconnecting using a server already in monitor->servers.  We don't
    // want to add it again or we'll end up with an infinite loop
    if (current == server) {
      return;
    }
    current = current->next_in_pool;
  }

  list = monitor->servers;
  server->next_in_pool = list;

  monitor->servers = server;
  monitor->num.in_use++;
}

void mongo_util_pool__rm_server_ptr(stack_monitor *monitor, mongo_server *server) {
  mongo_server *next, *prev, *current;

  next = server->next_in_pool;
  server->next_in_pool = 0;

  if (monitor->servers == 0) {
    return;
  }

  if (monitor->servers == server) {
    monitor->servers = next;
    monitor->num.in_use--;
    return;
  }

  prev = monitor->servers;
  current = monitor->servers->next_in_pool;
  while (current && current != server) {
    prev = current;
    current = current->next_in_pool;
  }

  if (current == server) {
    prev->next_in_pool = current->next_in_pool;
    monitor->num.in_use--;
  }
}

void mongo_util_pool__close_connections(stack_monitor *monitor) {
  mongo_server *current;

  // close all open connections
  current = monitor->servers;
  while (current) {
    mongo_util_pool__disconnect(monitor, current);
    monitor->num.in_use--;
    current = current->next_in_pool;
  }
  monitor->servers = 0;

  // remove any connections from the stack
  mongo_util_pool__stack_clear(monitor);
}

void mongo_util_pool__disconnect(stack_monitor *monitor, mongo_server *server) {
  int was_connected = server->connected;
  mongo_util_disconnect(server);
  if (was_connected &&
      (monitor->num.remaining < -1 || monitor->num.remaining > 0)) {
    monitor->num.remaining++;
  }
}

stack_monitor *mongo_util_pool__get_monitor(mongo_server *server TSRMLS_DC) {
  zend_rsrc_list_entry *le = 0;
  char *id;
  size_t len;

  if ((len = mongo_util_pool__get_id(server, &id TSRMLS_CC)) == FAILURE) {
    return 0;
  }

  if (zend_hash_find(&EG(persistent_list), id, len+1, (void**)&le) == FAILURE) {
    zend_rsrc_list_entry nle;
    stack_monitor *monitor;

    monitor = (stack_monitor*)malloc(sizeof(stack_monitor));
    if (!monitor) {
      efree(id);
      return 0;
    }

    memset(monitor, 0, sizeof(stack_monitor));

    // set pool size
    monitor->num.remaining = MonGlo(pool_size);

    // registering this links it to the dtor (mongo_util_pool_shutdown) so that
    // it can be auto-cleaned-up on shutdown
    nle.ptr = monitor;
    nle.type = le_pconnection;
    nle.refcount = 1;
    zend_hash_add(&EG(persistent_list), id, len+1, &nle, sizeof(zend_rsrc_list_entry), NULL);

    efree(id);
    return monitor;
  }

  efree(id);
  return le->ptr;
}

size_t mongo_util_pool__get_id(mongo_server *server, char **id TSRMLS_DC) {
  size_t len;

  if (!server) {
    return FAILURE;
  }

  len = spprintf(id, 0, "%s:%d.%s.%s.%s", server->host, server->port,
                 server->db ? server->db : "",
                 server->username ? server->username : "",
                 server->password ? server->password : "");

  return len;
}

int mongo_util_pool__connect(stack_monitor *monitor, mongo_server *server, zval *errmsg TSRMLS_DC) {
  if (monitor->num.remaining == 0) {
    ZVAL_STRING(errmsg, "no more connections in pool", 1);
    return FAILURE;
  }

  if (mongo_util_connect(server, monitor->timeout, errmsg) == SUCCESS &&
      // authenticate, if necessary
      mongo_util_connect_authenticate(server, errmsg TSRMLS_CC) == SUCCESS) {
    monitor->num.remaining--;
    server->connected = 1;
    return SUCCESS;
  }
  server->connected = 0;
  return FAILURE;
}

PHP_METHOD(Mongo, setPoolSize) {
  long size = -1, old = -1;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &size) == FAILURE) {
    RETURN_FALSE;
  }

  old = MonGlo(pool_size);
  MonGlo(pool_size) = size;
  RETURN_LONG(old);
}

PHP_METHOD(Mongo, getPoolSize) {
  RETURN_LONG(MonGlo(pool_size));
}

PHP_METHOD(Mongo, poolDebug) {
  HashPosition pointer;
  zend_rsrc_list_entry *le;

  array_init(return_value);

  for (zend_hash_internal_pointer_reset_ex(&EG(persistent_list), &pointer);
       zend_hash_get_current_data_ex(&EG(persistent_list), (void**) &le, &pointer) == SUCCESS;
       zend_hash_move_forward_ex(&EG(persistent_list), &pointer)) {
    zval *m;
    char *key;
    unsigned int key_len;
    unsigned long index;
    stack_monitor *monitor;

    if (!le || le->type != le_pconnection) {
      continue;
    }

    monitor = (stack_monitor*)le->ptr;

    MAKE_STD_ZVAL(m);
    array_init(m);

    add_assoc_long(m, "in use", monitor->num.in_use);
    add_assoc_long(m, "in pool", monitor->num.in_pool);
    add_assoc_long(m, "remaining", monitor->num.remaining);
    add_assoc_long(m, "timeout", monitor->timeout);

    if (zend_hash_get_current_key_ex(&EG(persistent_list), &key, &key_len, &index, 0, &pointer) == HASH_KEY_IS_STRING) {
      add_assoc_zval(return_value, key, m);
    }
    else {
      add_index_zval(return_value, index, m);
    }
  }

  // return_value is returned
}
