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

#ifndef MONGO_UTIL_LINK_H
#define MONGO_UTIL_LINK_H

/**
 * Handles link-level operations.
 */

/**
 * Get a slave socket.  Returns 0 and sets errmsg on failure.
 */
mongo_server* mongo_util_link_get_slave_socket(mongo_link *link, zval *errmsg TSRMLS_DC);

/**
 * Handle getting a connection from a single server, list of servers, or a replica
 * set.  Sets errmsg if it could not find a connection and returns 0.
 */
mongo_server* mongo_util_link_get_socket(mongo_link *link, zval *errmsg TSRMLS_DC);

/**
 * Disconnect all connections used by this link.
 */
void mongo_util_link_disconnect(mongo_link *link TSRMLS_DC);

void mongo_util_link_master_failed(mongo_link *link TSRMLS_DC);

/**
 * Indicate that a particular server that this link was using failed.
 */
int mongo_util_link_failed(mongo_link *link, mongo_server *server TSRMLS_DC);

#endif
