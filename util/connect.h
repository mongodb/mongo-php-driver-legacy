// connect.h
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

#ifndef MONGO_UTIL_CONN_H
#define MONGO_UTIL_CONN_H

#ifdef WIN32
#define MONGO_UTIL_DISCONNECT(socket)                           \
  shutdown((socket), 2);                                        \
  closesocket(socket);                                          \
  WSACleanup();
#else
#define MONGO_UTIL_DISCONNECT(socket) shutdown((socket), 2); close(socket);
#endif

/**
 * Individual socket connections.  Mostly helper functions for pool functions.
 */

/**
 * Find the max BSON size for this connection.
 */
void mongo_util_connect_buildinfo(zval *this_ptr TSRMLS_DC);

/**
 * Actually make the network connection.  Returns SUCCESS/FAILURE and sets
 * errmsg, never throws exceptions.
 */
int mongo_util_connect(mongo_server *server, int timeout, zval *errmsg);

/**
 * If this connection should be authenticated, try to authenticate.  Returns
 * SUCCESS/FAILURE and sets errmsg, never throws exceptions.
 */
int mongo_util_connect_authenticate(mongo_server *server, zval *errmsg TSRMLS_DC);

/**
 * Disconnect from a socket.
 */
int mongo_util_disconnect(mongo_server *server);

/**
 * Find the master connection.
 */
mongo_server* mongo_util_connect_get_master(mongo_link *link TSRMLS_DC);


/**
 * Get the address of a socket (internal).
 */
int mongo_util_connect__sockaddr(struct sockaddr *sa, int family, char *host, int port, zval *errmsg);

#endif
