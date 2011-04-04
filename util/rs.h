// rs.h
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

#ifndef MONGO_UTIL_RS_H
#define MONGO_UTIL_RS_H

/**
 * Replica set interface
 *
 * This figures out who the master is, finds good servers to read from, and
 * controls pinging all servers in the set.
 */


// ------------ Replica set interface ---------

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
int set_a_slave(mongo_link *link, char **errmsg);

/**
 * This uses the master connection to find the replSetStatus of the replica set.
 * The information is stored in a persistent hash (see util/hash.h) and used to
 * determine slave eligibility.
 *
 * TODO: this should be a per-member call that calls ismaster
 */
int get_heartbeats(zval *this_ptr, char **errmsg  TSRMLS_DC);

/**
 * Refresh the list of servers in the set.  Only will run on a given link once
 * every 5 seconds. 
 *
 * Calls ismaster on each member of the set until it finds someone with a hosts
 * and/or passives field.  Uses the first it finds to repopulate the list,
 * returning all old connections to their pools and creating new servers for
 * each host.  New servers do not yet fetch connections from their pools.
 */
void mongo_util_rs_refresh(mongo_link *link TSRMLS_DC);

/**
 * Ping servers to see who is readable.  Only will run on a given link once
 * every 5 seconds (separate counter from that used for mongo_util_rs_refresh).
 */
void mongo_util_rs_ping(mongo_link *link TSRMLS_DC);

// -------- Internal functions ----------

/**
 * Calls ismaster on the given server.
 */
zval* mongo_util_rs__ismaster(mongo_server *current TSRMLS_DC);

/**
 * Helper for __ismaster. Creates a cursor to be used for the command.
 */
zval* mongo_util_rs__create_fake_cursor(mongo_link *link TSRMLS_DC);

/**
 * Helper function for __ismaster response.
 *
 * Returns if the server is master (1 if master, 0 if not).
 */
int mongo_util_rs__get_ismaster(zval *response TSRMLS_DC);

/**
 * Helper function for __ismaster response.
 *
 * If the server knows who the master is, sets link->server_set->server to the
 * master server (fetching a connection from the pool) and returns SUCCESS if
 * the master was successfully fetched (FAILURE otherwise).
 */
int mongo_util_rs__another_master(zval *response, mongo_link *link TSRMLS_DC);

/**
 * Helper for rs_refresh. Clears the hosts, returning each host to the pool.
 * Takes the output of rs__ismaster and looks through the hosts and passives
 * fields (TODO: arbiter field) to build a new list of hosts.
 */
void mongo_util_rs__refresh_list(mongo_link *link, zval *response TSRMLS_DC);

/**
 * Helper for __refresh_list.  For each host in the given list, creates a new
 * mongo_server* for it in the link and populates the hosts hash with a null
 * entry for it.
 */
void mongo_util_rs__repopulate_hosts(zval **hosts, mongo_link *link TSRMLS_DC);

/**
 * For a given hostname+port, checks the link's server list to see if it already
 * exists.  If it does, it is returned.  If not, a new mongo_server is created
 * for it in the list and an entry is added to the hosts hash for it.
 *
 * Returns the mongo_server on success, 0 on failure.
 */
mongo_server* mongo_util_rs__find_or_make_server(char *host, mongo_link *link TSRMLS_DC);

#endif
