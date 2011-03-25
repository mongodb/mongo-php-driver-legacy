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
 * Get a master from this master's sockets.
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

int get_heartbeats(zval *this_ptr, char **errmsg  TSRMLS_DC);

int mongo_util_rs__refresh_members(zval *response, mongo_link *link);

int mongo_util_rs__get_ismaster(zval *response TSRMLS_DC);

int mongo_util_rs__another_master(zval *response, mongo_link *link TSRMLS_DC);

void mongo_util_rs__refresh_list(mongo_link *link, zval *response TSRMLS_DC);

void mongo_util_rs__repopulate_hosts(zval **hosts, mongo_link *link TSRMLS_DC);

zval* mongo_util_rs__call_ismaster(mongo_server *current, zval *cursor_zval TSRMLS_DC);

zval* mongo_util_rs__create_fake_cursor(mongo_link *link TSRMLS_DC);

mongo_server* mongo_util_rs__find_or_make_server(char *host, mongo_link *link TSRMLS_DC);

#endif
