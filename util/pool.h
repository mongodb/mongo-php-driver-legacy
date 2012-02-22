// pool.h
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


/**
 * Each pool is uniquely identified by:
 *
 * serverAddr:port.db.user.password
 *
 * So the default pool name is "localhost:27017...".  Each pool is stored in the
 * persistent list.
 *
 * Each "pool" is actually a stack of connections.  If someone requests a
 * connection from an empty pool, a new one is created and immediately given
 * out.  When a connection is released, it is pushed onto the stack.
 * Connections correspond to threads in the database, so it is more likely that
 * a recently used connection's thread is active.
 *
 * When a mongo_server gets a connection, its socket and connected fields must
 * be set.  This is done in get().
 *
 * The pool has a list of all of its connections, both on the stack and "in the
 * wild."  If failed() is called and the server seems to be down, every
 * connection associated with that pool is closed and every connection on the
 * stack is freed.  When clients use a connection for the first time after a
 * failure, they must fetch a new connection from the pool.
 *
 * Logging shouldn't be used until a monitor has been acquired, as that checks
 * that "server" is correctly initialized.
 */

#ifndef MONGO_UTIL_POOL_H
#define MONGO_UTIL_POOL_H

/**
 * Sometimes when we close a connection, we want to check all connections for
 * validity.  Sometimes we just want to close a single connection.
 */
#define DONT_CHECK_CONNS 0
#define CHECK_CONNS 1

// ------- Pool Structs -----------

typedef struct _stack_node {
  int socket;
  struct _stack_node *next;
} stack_node;

typedef struct {
  // timeout for connections
  time_t timeout;
  // total time this pool has spent waiting for connections to be recycled
  int waiting;

  // number of servers in the pool
  struct {
    int in_pool;
    int in_use;
    int total;
    int remaining;
  } num;

  // the actual pool
  stack_node *top;

  // a pointer to each of the server structs using a connection from this pool,
  // so we can disconnect them all if something goes wrong.
  mongo_server *servers;
} stack_monitor;

#define EVERYONE_DISCONNECTED 1
// TODO: make this heurisitic
#define MAX_POOL_SIZE 20

// ------- Pool Interface -----------

/**
 * Initialize the pool for a given server.  Only sets connection timeout if it
 * is non-zero (-1 for wait forever). This is called every time a new Mongo
 * instance is created, but it only creates a pool monitor the first time.
 */
int mongo_util_pool_init(mongo_server *server, time_t timeout TSRMLS_DC);

/**
 * Close the bad connection and open a new one.  Does nothing if a healthy
 * connection already exists.
 *
 * Returns SUCCESS or FAILURE.
 */
int mongo_util_pool_refresh(mongo_server *server, time_t timeout TSRMLS_DC);

/**
 * Fetch a connection from the pool, based on the criteria given. You can pass
 * in a NULL errmsg to not set the error message on failure.
 * @return SUCCESS or FAILURE
 */
int mongo_util_pool_get(mongo_server *server, zval *errmsg TSRMLS_DC);

/**
 * Return a connection to its pool.
 */
void mongo_util_pool_done(mongo_server *server TSRMLS_DC);

/**
 * Cleans out connection pool and attempts to reconnect this connection after an
 * operation has failed.
 *
 * This was originally less aggressive about closing connections, but it's too
 * difficult to tell what's a network blip vs. all connections are bad, so this
 * now cleans out the full pool.
 *
 * If this fails to reconnect the socket, it will change the server's state to
 * "down" (see server.h).
 *
 * DO NOT call this from within the IO lock, as it tries to reconnect to the
 * server (which can cause it to try to re-acquire the IO lock).
 */
int mongo_util_pool_failed(mongo_server *server TSRMLS_DC);

/**
 * Clean up all pools on shutdown, disconnect all connections.
 */
void mongo_util_pool_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC);

/**
 * Remove a connection from a pool.  This is done when a connection is manually
 * authenticated, to prevent polluting the pool.
 */
void mongo_util_pool_remove(mongo_server *server TSRMLS_DC);

/**
 * Closes a connection and removes it from the pool.
 */
void mongo_util_pool_close(mongo_server *server, int check_conns TSRMLS_DC);

// ------- Internal Functions -----------

/**
 * Pop a connection off of the stack.
 *
 * Sets server->connected, server->socket, and monitor->num.in_pool.
 */
int mongo_util_pool__stack_pop(stack_monitor *monitor, mongo_server *server TSRMLS_DC);

/**
 * Push a connection onto the stack.  Will start emptying pool (from the bottom)
 * if there are more than 50 (idle) connections.
 *
 * Sets server->connected and monitor->num.in_pool.
 */
void mongo_util_pool__stack_push(stack_monitor *monitor, mongo_server *server TSRMLS_DC);

/**
 * Close and remove all connections from the stack.
 */
void mongo_util_pool__stack_clear(stack_monitor *monitor TSRMLS_DC);

/**
 * Close all connections for a given monitor.
 */
void mongo_util_pool__close_connections(stack_monitor *monitor TSRMLS_DC);

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
 * If there aren't any more connections in the pool, this function will wait
 * up to monitor->timeout milliseconds.
 */
int mongo_util_pool__timeout(stack_monitor *monitor);

/**
 * Create a new connection.  Returns SUCCESS/FAILURE and sets errmsg, never
 * throws exceptions.
 */
int mongo_util_pool__connect(stack_monitor *monitor, mongo_server *server, zval *errmsg TSRMLS_DC);

/**
 * Close a connection. Increments monitor's num.remaining
 */
void mongo_util_pool__disconnect(stack_monitor *monitor, mongo_server *server TSRMLS_DC);

/**
 * Get this monitor for this server.
 */
stack_monitor *mongo_util_pool__get_monitor(mongo_server *server TSRMLS_DC);


// ------- External Functions -----------

void mongo_init_MongoPool(TSRMLS_D);

PHP_METHOD(MongoPool, setSize);
PHP_METHOD(MongoPool, getSize);
PHP_METHOD(MongoPool, info);

PHP_METHOD(Mongo, setPoolSize);
PHP_METHOD(Mongo, getPoolSize);
PHP_METHOD(Mongo, poolDebug);

#endif
