// rs.h
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

#ifndef MONGO_UTIL_RS_H
#define MONGO_UTIL_RS_H

/**
 * Replica set interface
 *
 * This figures out who the master is, finds good servers to read from, and
 * controls pinging all servers in the set.
 *
 * Aliases
 * -------
 * Suppose the user passes in "localhost" as one of the seeds.  We do not find
 * localhost in the list of hosts, but we should try to use localhost if we can
 * resolve that localhost actually refers to "serverN" in the host list.
 *
 */

/**
 * Replica set monitor server.  Wrapper for a mongo_server with rs_monitor meta
 * information.
 */
typedef struct _rsm_server {
  mongo_server *server;
  struct _rsm_server *next;
} rsm_server;

typedef struct _rs_monitor {
  time_t last_ping;
  time_t last_ismaster;

  char *name;
  char *username;
  char *password;
  char *db;

  // the current primary or 0
  mongo_server *primary;
  // a list of all servers in the replica set
  rsm_server *servers;
} rs_monitor;

typedef struct _rs_container {
  int owner;
  rs_monitor *monitor;
} rs_container;

#define MONGO_RS "replicaSet"
#define PHP_RS_RES_NAME "replica set ts"
#define MONGO_RS_TIMEOUT 200

// ------------ Replica set interface ---------

int mongo_util_rs_init(mongo_link *link TSRMLS_DC);

/**
 * Get a master, if possible.  Returns a pointer to the master on success and
 * 0 on failure.  Does not throw exceptions or set error message.  This may
 * refresh the hosts list, even if it doesn't find a master.
 */
mongo_server* mongo_util_rs_get_master(mongo_link *link TSRMLS_DC);

/**
 * This finds the next slave on the list to use for reads. If it cannot find a
 * secondary and the primary is down, it will return FAILURE.  Otherwise, it
 * returns RS_SECONDARY if it is connected to a slave and RS_PRIMARY if it is
 * connected to the master.
 *
 * It choses a random slave between 0 & (number of secondaries-1).
 */
int mongo_util_rs__set_slave(mongo_link *link, char **errmsg TSRMLS_DC);

void mongo_util_rs_ping(mongo_link *link TSRMLS_DC);

/**
 * Clears the hosts, returning each host to the pool. Takes the output of
 * rs__ismaster and looks through the hosts and passives fields to build a new
 * list of hosts.
 */
void mongo_util_rs_refresh(rs_monitor *monitor, time_t now TSRMLS_DC);

/**
 * Free ping time.
 */
void mongo_util_rs_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC);

// -------- Internal functions ----------

/**
 * This finds if there are any known replica set monitors for the given seeds.
 * It should try very hard to find any duplicate monitors, so we aren't
 * monitoring the same set in two threads.
 */
rs_monitor* mongo_util_rs__get_monitor(mongo_link *link TSRMLS_DC);

void mongo_util_rs__ping(rs_monitor *monitor TSRMLS_DC);

/**
 * Calls ismaster on the given server.
 */
zval* mongo_util_rs__cmd(char *cmd, mongo_server *current TSRMLS_DC);

/**
 * Helper function for __ismaster response.
 *
 * Returns if the server is master (1 if master, 0 if not).
 */
int mongo_util_rs__get_ismaster(zval *response TSRMLS_DC);

#endif
