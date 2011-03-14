// pool.h
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


/**
 * The server pool is actually a hashtable of pools, uniquely identified by:
 *
 * serverAddr:port.db.user.password
 *
 * The hashtable is stored in the persistent list.
 *
 * Each "pool" is actually a stack of connections.  If someone requests a
 * connection from an empty pool, a new one is created and immediately given
 * out.  When a connection is released, it is pushed onto the stack.
 *
 * When a mongo_server gets a connection, its socket and connected fields must
 * be set.  This is done in get().
 *
 * The pool has a list of all of its connections, both on the stack and "in the
 * wild."  If failed() is called, every connection associated with that pool is
 * closed and every connection on the stack is freed.  When clients use a
 * connection for the first time after a failure, they must fetch a new
 * connection from the pool.
 *
 * TODO: modify MongoDB::authenticate to remove the server from the pool
 * TODO: only disconnect on two failures in a row (to prevent spurious disconnects)
 */

#ifndef MONGO_UTIL_POOL_H
#define MONGO_UTIL_POOL_H

// ------- Pool Structs -----------

typedef struct _stack_node {
  int socket;
  struct _stack_node *next;
} stack_node;

typedef struct {
  // timeout for connections
  time_t timeout;
  
  // number of servers in the pool
  struct {
    int in_pool;
    int in_use;
  } num;
  
  stack_node *top;
  // a pointer to each of the server structs using a connection from this pool,
  // so we can disconnect them all if something goes wrong.
  mongo_server *servers;
} stack_monitor;

#define CONNECTION_POOLS "mongoConnectionPool"
// TODO: make this heurisitic
#define INITIAL_POOL_SIZE 10

// ------- Pool Interface -----------

/**
 * Initialize the pool for a given server.  Only sets connection timeout if it
 * is non-zero (-1 for wait forever).
 */
int mongo_util_pool_init(mongo_server *server, time_t timeout TSRMLS_DC);

/**
 * Fetch a connection from the pool, based on the criteria given.
 * @return the connection, 0 on failure
 */
int mongo_util_pool_get(mongo_server *server, zval *errmsg TSRMLS_DC);

/**
 * Return a connection to its pool.
 */
void mongo_util_pool_done(mongo_server *server TSRMLS_DC);

/**
 * Attempts to reconnect to the database after an operation has failed.  If it
 * is unable to reconnect, it assumes something is *really* wrong and clears and
 * disconnects the connection stack.
 */
void mongo_util_pool_failed(mongo_server *server TSRMLS_DC);

/**
 * Clean up all pools on shutdown, disconnect all connections.
 */
void mongo_util_pool_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC);


// ------- Internal Functions -----------

stack_node* mongo_util_pool__stack_pop(stack_monitor *monitor);
void mongo_util_pool__stack_push(stack_monitor *monitor, mongo_server *server);

/**
 * Close all connections for a given monitor.
 */
void mongo_util_pool__close_connections(stack_monitor *monitor);

/**
 * Remove a server reference from this monitor.
 */
void mongo_util_pool__rm_server_ptr(stack_monitor *monitor, mongo_server *server);

/**
 * Add a server reference for this pointer.
 */
void mongo_util_pool__add_server_ptr(stack_monitor *monitor, mongo_server *server);

/**
 * Creates the identifying string for this server's hash table entry.
 */
size_t mongo_util_pool__get_id(mongo_server *server, char **id TSRMLS_DC);

/**
 * Create a new connection.  Returns SUCCESS/FAILURE and sets errmsg, never
 * throws exceptions.
 */
int mongo_util_pool__connect(mongo_server *server, time_t timeout, zval *errmsg TSRMLS_DC);

/**
 * Get this monitor for this server.
 */
stack_monitor *mongo_util_pool__get_monitor(mongo_server *server TSRMLS_DC);

/**
 * Get all connection pools for this instance.
 */
HashTable *mongo_util_pool__get_connection_pools(TSRMLS_D);

// ------- External (debug) Functions -----------

PHP_FUNCTION(mongoPoolDebug);

#endif
