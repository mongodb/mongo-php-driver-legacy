// server.h
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

#ifndef MONGO_UTIL_SERVER_H
#define MONGO_UTIL_SERVER_H

/**
 * Handles information about a single server.
 */

typedef struct _server_info {
  // if this has been pinged at least once
  int pinged;

  int max_bson_size;
  int readable;
  int master;

  // for pinging rs slaves
  int ping;
  time_t last_ping;
} server_info;

#define MONGO_SERVER_INFO "server_info"
#define MONGO_SERVER_PING INT_MAX
#define MONGO_SERVER_BSON (4*1024*1024)

#define MONGO_PING_INTERVAL 5

/**
 * If it's been PING_INTERVAL since we last pinged this server, calls isMaster
 * on this server.
 *
 * Return values are a little weird: returns SUCCESS if this is a master,
 * FAILURE if it is not.
 */
int mongo_util_server_ping(mongo_server *server, time_t now TSRMLS_DC);

/**
 * Find the ping time for the given server.
 */
int mongo_util_server_get_ping_time(mongo_server *server TSRMLS_DC);

/**
 * Store a new ping time for the given server.
 */
int mongo_util_server__set_ping(server_info *info, struct timeval start, struct timeval end);

/**
 * Fetch the server info struct for a given server.
 */
server_info* mongo_util_server__get_info(mongo_server *server TSRMLS_DC);

void mongo_util_server_down(mongo_server *server TSRMLS_DC);

void mongo_util_server_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC);

/**
 * Sets if this server is readable.
 */
int mongo_util_server_set_readable(mongo_server *server, zend_bool readable TSRMLS_DC);
int mongo_util_server_get_readable(mongo_server *server TSRMLS_DC);

int mongo_util_server_get_bson_size(mongo_server *server TSRMLS_DC);

#ifdef WIN32
void gettimeofday(struct timeval *t, void* tz);
#endif

#endif
