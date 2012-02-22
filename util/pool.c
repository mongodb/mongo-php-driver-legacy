// pool.c
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

#include <php.h>

#ifndef WIN32
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#endif

#include "../php_mongo.h"
#include "hash.h"
#include "pool.h"
#include "connect.h"
#include "server.h"
#include "log.h"
#include "rs.h"

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_Pool;

extern int le_pconnection;

#if WIN32
HANDLE pool_mutex;
#else
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/**
 * When there's a failure on a connection, we try pinging mongod on another
 * connection. If that ping fails, we assume that the server bounced or similar
 * and close all connections.  If we get a successful ping response, we assume
 * just our socket went bad and leave the other connections open.
 */
static void get_other_conn(mongo_server *server, mongo_server *other, stack_monitor *monitor);

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

int mongo_util_pool_refresh(mongo_server *server, time_t timeout TSRMLS_DC) {
  if (server->connected) {
    return SUCCESS;
  }

  // make sure we're disconnected before reconnecting
  mongo_util_pool_close(server, CHECK_CONNS TSRMLS_CC);

  if (mongo_util_pool_init(server, timeout TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  if (mongo_util_pool_get(server, 0 TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  // if we get here, we successfully reconnected
  return SUCCESS;
}

int mongo_util_pool_get(mongo_server *server, zval *errmsg TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  mongo_log(MONGO_LOG_POOL, MONGO_LOG_FINE TSRMLS_CC, "%s: pool get (%p)", server->label, monitor);

  // get connection from pool or create new
  if (mongo_util_pool__stack_pop(monitor, server TSRMLS_CC) == SUCCESS ||
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

  mongo_log(MONGO_LOG_POOL, MONGO_LOG_FINE TSRMLS_CC, "%s: pool done (%p)", server->label, monitor);

  // clean up reference to server (nothing needs to be freed)
  mongo_util_pool__rm_server_ptr(monitor, server);

  // if this is disconnected, do not add it to the pool
  if (server->connected) {
    mongo_util_pool__stack_push(monitor, server TSRMLS_CC);
  }
}

void mongo_util_pool_remove(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    // if we couldn't push this, close the connection
    mongo_util_disconnect(server);
    return;
  }

  mongo_log(MONGO_LOG_POOL, MONGO_LOG_FINE TSRMLS_CC, "%s: pool remove (%p)", server->label, monitor);

  // clean up reference to server (nothing needs to be freed)
  mongo_util_pool__rm_server_ptr(monitor, server);
}

void mongo_util_pool_close(mongo_server *server, int check_conns TSRMLS_DC) {
  stack_monitor *monitor;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    mongo_util_disconnect(server);
    return;
  }

  mongo_log(MONGO_LOG_POOL, MONGO_LOG_FINE TSRMLS_CC, "%s: pool close (%p)", server->label, monitor);

  mongo_util_pool__disconnect(monitor, server TSRMLS_CC);

  // clean up reference to server (nothing needs to be freed)
  mongo_util_pool__rm_server_ptr(monitor, server);
}

static void get_other_conn(mongo_server *server, mongo_server *other, stack_monitor *monitor) {
  mongo_server *current = 0;
  stack_node *node = 0;

  LOCK(pool);

  // look through pool conns
  node = monitor->top;
  if (node) {
    other->connected = 1;
    other->socket = node->socket;

    UNLOCK(pool);
    return;
  }

  // look through in-use conns
  current = monitor->servers;
  while (current) {
    if (current != server) {
      other->connected = 1;
      other->socket = current->socket;

      UNLOCK(pool);
      return;
    }

    current = current->next_in_pool;
  }

  UNLOCK(pool);

  other->connected = 0;
  other->socket = 0;
}

int mongo_util_pool_failed(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;
  zval *errmsg;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    mongo_util_disconnect(server);
    return FAILURE;
  }

  mongo_log(MONGO_LOG_POOL, MONGO_LOG_FINE TSRMLS_CC, "%s: pool fail (%p)", server->label, monitor);

  mongo_util_pool__close_connections(monitor TSRMLS_CC);
  // just to be sure
  mongo_util_pool__disconnect(monitor, server TSRMLS_CC);

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
  mongo_util_pool__close_connections(monitor TSRMLS_CC);
  pefree(monitor, 1);
  rsrc->ptr = 0;
}

int mongo_util_pool__stack_pop(stack_monitor *monitor, mongo_server *server TSRMLS_DC) {
  stack_node *node;

  LOCK(pool);

  node = monitor->top;

  // check that monitor->pop != NULL and pop stack
  if (!node) {
    UNLOCK(pool);

    server->connected = 0;
    return FAILURE;
  }

  monitor->top = node->next;
  monitor->num.in_pool--;

  // theoretically, all servers in the pool should be connected
  server->connected = 1;
  server->socket = node->socket;

  pefree(node, 1);

  UNLOCK(pool);

  // label is not set when stack_clear calls this
  if (server->label) {
    mongo_log(MONGO_LOG_POOL, MONGO_LOG_FINE TSRMLS_CC, "%s: found in pool (%p)", server->label, monitor);
  }

  return SUCCESS;
}

void mongo_util_pool__stack_push(stack_monitor *monitor, mongo_server *server TSRMLS_DC) {
  stack_node *node;

  if (!server->connected) {
    return;
  }

  LOCK(pool);

  node = (stack_node*)pemalloc(sizeof(stack_node), 1);

  node->socket = server->socket;

  node->next = monitor->top;
  monitor->top = node;

  monitor->num.in_pool++;
  server->connected = 0;

  // don't keep more than MAX_POOL_SIZE connections around
  node = monitor->top;
  if (monitor->num.in_pool > MAX_POOL_SIZE) {
    int count = 0, removed = 0;
    stack_node *next;

    mongo_log(MONGO_LOG_POOL, MONGO_LOG_INFO TSRMLS_CC, "%s: trimming pool from %d to %d (%p)",
              server->label, monitor->num.in_pool, MAX_POOL_SIZE, monitor);

    while (node && count < MAX_POOL_SIZE-1) {
      node = node->next;
      count++;
    }

    if (count < (MAX_POOL_SIZE-1) || !node) {
      mongo_log(MONGO_LOG_POOL, MONGO_LOG_WARNING TSRMLS_CC, "%s: BAD POOL SIZE: %d, actually %d (%p)",
                server->label, monitor->num.in_pool, count, monitor);
      UNLOCK(pool);
      return;
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
      removed++;
    }

    mongo_log(MONGO_LOG_POOL, MONGO_LOG_INFO TSRMLS_CC, "%s: trimmed pool by %d (%p)",
              server->label, removed, monitor);
  }

  UNLOCK(pool);
}

void mongo_util_pool__stack_clear(stack_monitor *monitor TSRMLS_DC) {
  // holder for popping sockets
  mongo_server temp;
  temp.label = 0;
  temp.owner = getpid();

  while (mongo_util_pool__stack_pop(monitor, &temp TSRMLS_CC) == SUCCESS) {
    mongo_util_pool__disconnect(monitor, &temp TSRMLS_CC);
  }
  monitor->top = 0;
}


void mongo_util_pool__add_server_ptr(stack_monitor *monitor, mongo_server *server) {
  mongo_server *list, *current;

  LOCK(pool);

  current = monitor->servers;
  while (current) {
    // we are reconnecting using a server already in monitor->servers.  We don't
    // want to add it again or we'll end up with an infinite loop
    if (current == server) {
      UNLOCK(pool);
      return;
    }
    current = current->next_in_pool;
  }

  list = monitor->servers;
  server->next_in_pool = list;

  monitor->servers = server;
  monitor->num.in_use++;

  UNLOCK(pool);
}

void mongo_util_pool__rm_server_ptr(stack_monitor *monitor, mongo_server *server) {
  mongo_server *next, *prev, *current;

  LOCK(pool);

  next = server->next_in_pool;
  server->next_in_pool = 0;

  if (monitor->servers == 0) {
    UNLOCK(pool);
    return;
  }

  if (monitor->servers == server) {
    monitor->servers = next;
    monitor->num.in_use--;

    UNLOCK(pool);
    return;
  }

  prev = monitor->servers;
  current = monitor->servers->next_in_pool;
  while (current && current != server) {
    prev = current;
    current = current->next_in_pool;
  }

  if (current == server) {
    prev->next_in_pool = next;
    monitor->num.in_use--;
  }

  UNLOCK(pool);
}

void mongo_util_pool__close_connections(stack_monitor *monitor TSRMLS_DC) {
  mongo_server *current;

  LOCK(pool);

  // close all open connections
  current = monitor->servers;
  while (current) {
    mongo_util_pool__disconnect(monitor, current TSRMLS_CC);
    monitor->num.in_use--;
    current = current->next_in_pool;
  }
  monitor->servers = 0;

  UNLOCK(pool);

  // remove any connections from the stack
  mongo_util_pool__stack_clear(monitor TSRMLS_CC);
}

void mongo_util_pool__disconnect(stack_monitor *monitor, mongo_server *server TSRMLS_DC) {
  int was_connected = server->connected;

  // kill any cursor associated with this connection before deleting it
  mongo_cursor_free_le(server, MONGO_SERVER TSRMLS_CC);

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

  LOCK(pool);

  if (zend_hash_find(&EG(persistent_list), id, len+1, (void**)&le) == FAILURE) {
    zend_rsrc_list_entry nle;
    stack_monitor *monitor;

    monitor = (stack_monitor*)pemalloc(sizeof(stack_monitor), 1);
    memset(monitor, 0, sizeof(stack_monitor));

    // set pool size
    monitor->num.total = monitor->num.remaining = MonGlo(pool_size);

    // registering this links it to the dtor (mongo_util_pool_shutdown) so that
    // it can be auto-cleaned-up on shutdown
    nle.ptr = monitor;
    nle.type = le_pconnection;
    nle.refcount = 1;
    zend_hash_add(&EG(persistent_list), id, len+1, &nle, sizeof(zend_rsrc_list_entry), NULL);

    UNLOCK(pool);

    efree(id);
    return monitor;
  }

  UNLOCK(pool);

  efree(id);
  return le->ptr;
}

size_t mongo_util_pool__get_id(mongo_server *server, char **id TSRMLS_DC) {
  size_t len;

  if (!server) {
    return FAILURE;
  }

  len = spprintf(id, 0, "%s:%d.%s.%s.%s.%d", server->host, server->port,
                 server->db ? server->db : "",
                 server->username ? server->username : "",
                 server->password ? server->password : "",
                 getpid());

  return len;
}

int mongo_util_pool__timeout(stack_monitor *monitor) {
  int remaining, timeout = monitor->timeout;

  // timeout = -1 returns immediately, so we don't sleep forever if no pool
  // connections become available

  LOCK(pool);
  remaining = monitor->num.remaining;
  UNLOCK(pool);

  while (timeout > 0 && remaining == 0) {
#ifdef WIN32
    // windows sleep takes milliseconds
    Sleep(10);
#else
    {
      // usleep is deprecated
      struct timespec wait;

      wait.tv_sec = 0;
      wait.tv_nsec = 10000000;

      nanosleep(&wait, 0);
    }
#endif
    timeout -= 10;
    monitor->waiting += 10;

    LOCK(pool);
    remaining = monitor->num.remaining;
    UNLOCK(pool);
  }

  return remaining != 0 ? SUCCESS : FAILURE;
}

int mongo_util_pool__connect(stack_monitor *monitor, mongo_server *server, zval *errmsg TSRMLS_DC) {
  mongo_log(MONGO_LOG_POOL, MONGO_LOG_FINE TSRMLS_CC, "%s: pool connect (%p)", server->label, monitor);

  if (mongo_util_pool__timeout(monitor) == FAILURE) {
    if (errmsg) {
      ZVAL_STRING(errmsg, "no more connections in pool", 1);
    }
    return FAILURE;
  }

  if (mongo_util_connect(server, monitor->timeout, errmsg) == FAILURE) {
    server->connected = 0;
    return FAILURE;
  }

  // authenticate, if necessary
  if (mongo_util_connect_authenticate(server, errmsg TSRMLS_CC) == FAILURE) {
    mongo_util_disconnect(server);
    return FAILURE;
  }

  monitor->num.remaining--;
  if (monitor->num.total > 0 && monitor->num.remaining < 0) {
    monitor->num.remaining = 0;
  }

  server->connected = 1;
  return SUCCESS;
}

static zend_function_entry MongoPool_methods[] = {
  PHP_ME(MongoPool, info, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoPool, setSize, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoPool, getSize, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  {NULL, NULL, NULL}
};

void mongo_init_MongoPool(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoPool", MongoPool_methods);
  mongo_ce_Pool = zend_register_internal_class(&ce TSRMLS_CC);
}

PHP_METHOD(MongoPool, setSize) {
  long size = -1, old = -1;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &size) == FAILURE) {
    RETURN_FALSE;
  }

  old = MonGlo(pool_size);
  MonGlo(pool_size) = size;
  RETURN_LONG(old);
}

PHP_METHOD(MongoPool, getSize) {
  RETURN_LONG(MonGlo(pool_size));
}

PHP_METHOD(MongoPool, info) {
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
    add_assoc_long(m, "total", monitor->num.total);
    add_assoc_long(m, "timeout", monitor->timeout);
    add_assoc_long(m, "waiting", monitor->waiting);

    if (zend_hash_get_current_key_ex(&EG(persistent_list), &key, &key_len, &index, 0, &pointer) == HASH_KEY_IS_STRING) {
      add_assoc_zval(return_value, key, m);
    }
    else {
      add_index_zval(return_value, index, m);
    }
  }

  // return_value is returned
}

PHP_METHOD(Mongo, setPoolSize) {
  MONGO_METHOD_BASE(MongoPool, setSize)(1, return_value, NULL, 0, 0 TSRMLS_CC);
}

PHP_METHOD(Mongo, getPoolSize) {
  MONGO_METHOD_BASE(MongoPool, getSize)(1, return_value, NULL, 0, 0 TSRMLS_CC);
}

PHP_METHOD(Mongo, poolDebug) {
  MONGO_METHOD_BASE(MongoPool, info)(1, return_value, NULL, 0, 0 TSRMLS_CC);
}
