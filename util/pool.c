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

#include "php_mongo.h"
#include "hash.h"
#include "pool.h"


int mongo_util_pool_init(mongo_server *server, time_t timeout TSRMLS_DC) {
  stack_monitor *monitor;
  
  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }
  if (timeout) {
    monitor->timeout = timeout;
  }
  
  // maybe initialize pool to a certain size?
  return SUCCESS;
}

int mongo_util_pool_get(mongo_server *server, zval *errmsg TSRMLS_DC) {
  stack_monitor *monitor;
  stack_node *node;
  
  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }
  
  node = monitor->top;
  
  // null <- [conn1]
  //          ^node
  if (node) {
    stack_node *target;

    target = mongo_util_pool__stack_pop(monitor);
    monitor->num.in_use++;
    
    server->socket = target->socket;
    free(target);
    
    return SUCCESS;
  }
  // create a new connection, no point in adding to stack
  else if (mongo_util_pool__connect(server, monitor->timeout, errmsg TSRMLS_CC) == SUCCESS) {
    // add this server to the list of monitored servers for this pool
    mongo_util_pool__add_server_ptr(monitor, server);    
    return SUCCESS;
  }
  
  return FAILURE;
}

void mongo_util_pool_done(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;
  stack_node *node;

  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    // if we couldn't push this, close the connection
    mongo_util_disconnect(server);
    return;
  }

  // clean up reference to server (nothing needs to be freed)
  mongo_util_pool__rm_server_ptr(monitor, server);
  
  // move to stack
  mongo_util_pool__stack_push(monitor, server);
}

void mongo_util_pool_failed(mongo_server *server TSRMLS_DC) {
  stack_monitor *monitor;
  zval *errmsg;
  
  if ((monitor = mongo_util_pool__get_monitor(server TSRMLS_CC)) == 0) {
    return;
  }

  // some routers cut off connections after x time, so we don't want to drop all
  // of the connections unless we can't reconnect
  mongo_util_disconnect(server);

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);
  if (mongo_util_pool__connect(server, monitor->timeout, errmsg TSRMLS_CC) == FAILURE) {
    mongo_util_pool__close_connections(monitor);
  }
  zval_ptr_dtor(&errmsg);
}

void mongo_util_pool_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  HashTable *hash;
  HashPosition pointer;
  stack_monitor *monitor;

  hash = (HashTable*)rsrc->ptr;
  
  for (zend_hash_internal_pointer_reset_ex(hash, &pointer); 
       zend_hash_get_current_data_ex(hash, (void**) &monitor, &pointer) == SUCCESS; 
       zend_hash_move_forward_ex(hash, &pointer)) {
    mongo_util_pool__close_connections(monitor);
  }
}

stack_node* mongo_util_pool__stack_pop(stack_monitor *monitor) {
  stack_node *node;

  node = monitor->top;
    
  // pop stack
  monitor->top = node->next;
  
  monitor->num.in_pool--;
  
  return node;
}

void mongo_util_pool__stack_push(stack_monitor *monitor, mongo_server *server) {
  stack_node *node;
  
  node = (stack_node*)malloc(sizeof(stack_node));
  
  node->socket = server->socket;
  node->next = monitor->top;
  monitor->top = node;

  monitor->num.in_pool++;
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
  mongo_server *prev, *current;
  
  if (monitor->servers == 0) {
    return;
  }
  
  if (monitor->servers == server) {
    monitor->servers = server->next_in_pool;
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
  stack_node *top;

  // close all open connections
  current = monitor->servers;
  while (current) {
    mongo_util_disconnect(current);
    monitor->num.in_use--;
    current = current->next_in_pool;
  }
  monitor->servers = 0;

  // remove any connections from the stack
  top = monitor->top;
  while (top) {
    stack_node *current;

    current = mongo_util_pool__stack_pop(monitor);
    MONGO_UTIL(current->socket);

    free(current);
  }
  monitor->top = 0;
}

HashTable *mongo_util_pool__get_connection_pools() {
  zend_rsrc_list_entry *le = 0, le_struct;
  
  // get persistent connection hash
  if (zend_hash_find(&EG(persistent_list), "mongoConnectionPool",
                     sizeof("mongoConnectionPool"), (void**)&le) == FAILURE) {
    le = &le_struct;
    le->ptr = 0;

    if (zend_hash_update(&EG(persistent_list), "mongoConnectionPool", sizeof("mongoConnectionPool"),
                         (void*)le, sizeof(zend_rsrc_list_entry), NULL) == FAILURE) {
      return 0;
    }
  }
  
  if (!le->ptr) {
    le->ptr = (HashTable*)malloc(sizeof(HashTable));
    if (!le->ptr) {
      return 0;
    }

    zend_hash_init(le->ptr, 8, 0, mongo_util_hash_dtor, 1);
  }

  return le->ptr;
}

stack_monitor *mongo_util_pool__get_monitor(mongo_server *server TSRMLS_DC) {
  HashTable *pools;
  stack_monitor *mptr;
  char *id = mongo_util_pool__get_id(server TSRMLS_CC);

  if ((pools = mongo_util_pool__get_connection_pools()) == 0) {
    return 0;
  }
  
  if (zend_hash_find(pools, id, sizeof(id), (void**)&mptr) == FAILURE) {
    stack_monitor *monitor;
    monitor = (stack_monitor*)malloc(sizeof(stack_monitor));
    if (!monitor) {
      return 0;
    }

    memset(monitor, 0, sizeof(stack_monitor));
    
    if (zend_hash_add(pools, id, sizeof(id), monitor, sizeof(stack_monitor), NULL) == FAILURE) {
      free(monitor);
      return 0;
    }

    return monitor;
  }
  
  return mptr;
}

char* mongo_util_pool__get_id(mongo_server *server TSRMLS_DC) {
  char *id;

  spprintf(&id, 0, "%s:%d.%s.%s.%s", server->host, server->port,
           server->db ? server->db : "",
           server->username ? server->username : "",
           server->password ? server->password : "");
  
  return id;
}

int mongo_util_pool__connect(mongo_server *server, time_t timeout, zval *errmsg TSRMLS_DC) {
  if (mongo_util_connect(server, timeout, errmsg) == SUCCESS &&
      // authenticate, if necessary
      mongo_util_connect_authenticate(server, errmsg TSRMLS_CC) == SUCCESS) {
    return SUCCESS;
  }
  return FAILURE;
}
