// server.h
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

#ifndef MONGO_UTIL_SERVER_H
#define MONGO_UTIL_SERVER_H

/**
 * Handles state information about a single server.
 *
 * If someone connects to a server and the driver can figure out that this
 * server is either equivalent to one already connected to or referred to by a
 * different name in the replica set, then there will be one "info guts" struct
 * holding information about this server.  Other info structs will point to this
 * structs guts.
 *
 * Server info is derived by two calls: once ever ISMASTER_INTERVAL, ismaster is
 * called to determine if this is a primary or secondary, BSON object size, etc.
 * Once every PING_INTERVAL seconds, ping is called to just determine if the
 * server is up or down.  We assume it's in the same state it was on last
 * ismaster check.
 *
 * TODO:
 * If a primary gets a "not master" error, it should become unreadable and
 * unwritable until the next ismaster call.  If a slave gets a "not secondary"
 * error, it should become unreadable until the next ismaster.
 */

typedef struct _server_guts {
  // if this has been pinged at least once
  int pinged;

  int max_bson_size;
  int readable;
  int master;

  // for pinging rs slaves
  int ping;
  int bucket;

  time_t last_ping;
  time_t last_ismaster;
} server_guts;

/**
 * Multiple server_info structs might point at the same guts, so owner is only
 * set when the struct owns the guts.
 */
typedef struct _server_info {
  int owner;
  server_guts *guts;
} server_info;


#define MONGO_SERVER_INFO "server_info"
#define MONGO_SERVER_PING INT_MAX
#define MONGO_SERVER_BSON (4*1024*1024)

#define MONGO_PING_INTERVAL 5
#define MONGO_ISMASTER_INTERVAL 60

// ------- Server Info Interface -----------

/**
 * This creates a non-persistent copy of source in dest and then returns dest
 * (to safely handle the case where a null pointer is passed in). If dest was
 * set, it will free the current dest (including returning its connection to the
 * pool) and recreate it in source's image.  It will also attempt to fetch a new
 * connection for from the pool.
 *
 * If persist is 1, this assumes that dest is persistent and creates a new
 * persistent mongo_server.  If persist is 0, it creates a non-persistent
 * mongo_server.
 *
 * Always returns a valid pointer, cannot return 0.
 */
mongo_server* mongo_util_server_copy(const mongo_server *source, mongo_server *dest, int persist TSRMLS_DC);

/**
 * Returns 0 if host1 is an alias of host2.  Returns non-zero otherwise.
 */
int mongo_util_server_cmp(char *host1, char *host2 TSRMLS_DC);

/**
 * If it's been PING_INTERVAL since we last pinged this server, calls ping
 * on this server.  Sets the ping time.
 *
 * Returns SUCCESS if this server is readable, failure otherwise.
 */
int mongo_util_server_ping(mongo_server *server, time_t now TSRMLS_DC);

/**
 * If it's been ISMASTER_INTERVAL since we last pinged this server, calls isMaster
 * on this server.
 *
 * Return values are a little weird: returns SUCCESS if this is a master,
 * FAILURE if it is not.
 */
int mongo_util_server_ismaster(server_info *info, mongo_server *server, time_t now TSRMLS_DC);

/**
 * Store a new ping time for the given server.
 */
int mongo_util_server__set_ping(server_info *info, struct timeval start, struct timeval end);

/**
 * Set this server to be in the "down" state: neither primary nor readable.
 */
void mongo_util_server_down(mongo_server *server TSRMLS_DC);

/**
 * Returns 1 if server is primary, 2 if secondary, 0 if neither.
 */
int mongo_util_server_get_state(mongo_server *server TSRMLS_DC);

/**
 * Sets if this server is readable.
 */
int mongo_util_server_set_readable(mongo_server *server, zend_bool readable TSRMLS_DC);
int mongo_util_server_get_readable(mongo_server *server TSRMLS_DC);

int mongo_util_server_get_bson_size(mongo_server *server TSRMLS_DC);

/**
 * Gets the "bucket" this server is in based on ping time.
 */
int mongo_util_server_get_bucket(mongo_server *server TSRMLS_DC);

void mongo_util_server_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC);

// ------- Internal Functions -----------

/**
 * If this server hasn't been pinged before, pings it.
 */
void mongo_util_server__prime(server_info *info, mongo_server *server TSRMLS_DC);

/**
 * Fetch the server info struct for a given server.
 */
server_info* mongo_util_server__get_info(mongo_server *server TSRMLS_DC);

/**
 * Calls _isSelf to determine if this server matches any other servers.  If it
 * does, uses the other server's info as its own.
 */
zend_rsrc_list_entry* mongo_util_server__other_le(mongo_server *server TSRMLS_DC);


#ifdef WIN32
void gettimeofday(struct timeval *t, void* tz);
#endif

PHP_METHOD(Mongo, serverInfo);

#endif
