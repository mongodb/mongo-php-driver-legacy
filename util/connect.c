// connect.c
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

#include <php.h>
#include <zend_exceptions.h>

#ifdef WIN32
#include <winsock2.h>
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/un.h>
#endif

#include "../php_mongo.h"
#include "../mongo.h"
#include "../db.h"
#include "connect.h"
#include "log.h"

extern zend_class_entry *mongo_ce_Mongo;
ZEND_EXTERN_MODULE_GLOBALS(mongo);

int mongo_util_connect(mongo_server *server, int timeout, zval *errmsg) {
  struct sockaddr* sa;
  struct sockaddr_in si;
  socklen_t sn;
  int family;
  struct timeval tval;
  int connected = FAILURE, status = FAILURE;

#ifdef WIN32
  WORD version;
  WSADATA wsaData;
  int size, error;
  u_long no = 0;
  const char yes = 1;
#else
  struct sockaddr_un su;
  uint size;
  int yes = 1;
#endif

  server->owner = getpid();

#ifdef WIN32
  family = AF_INET;
  sa = (struct sockaddr*)(&si);
  sn = sizeof(si);

  version = MAKEWORD(2,2);
  error = WSAStartup(version, &wsaData);

  if (error != 0) {
    return FAILURE;
  }

  // create socket
  server->socket = socket(family, SOCK_STREAM, 0);
  if (server->socket == INVALID_SOCKET) {
    if (errmsg) {
      ZVAL_STRING(errmsg, "Could not create socket", 1);
    }
    return FAILURE;
  }

#else
  // domain socket
  if (server->port==0) {
    family = AF_UNIX;
    sa = (struct sockaddr*)(&su);
    sn = sizeof(su);
  } else {
    family = AF_INET;
    sa = (struct sockaddr*)(&si);
    sn = sizeof(si);
  }

  // create socket
  if ((server->socket = socket(family, SOCK_STREAM, 0)) == FAILURE) {
    if (errmsg) {
      ZVAL_STRING(errmsg, strerror(errno), 1);
    }
    return FAILURE;
  }
#endif

  // timeout: set in ms or default of 20
  tval.tv_sec = timeout <= 0 ? 20 : timeout / 1000;
  tval.tv_usec = timeout <= 0 ? 0 : (timeout % 1000) * 1000;

  // get addresses
  if (mongo_util_connect__sockaddr(sa, family, server->host, server->port, errmsg) == FAILURE) {
    mongo_util_disconnect(server);
    // errmsg set in mongo_get_sockaddr
    return FAILURE;
  }

  setsockopt(server->socket, SOL_SOCKET, SO_KEEPALIVE, &yes, INT_32);
  setsockopt(server->socket, IPPROTO_TCP, TCP_NODELAY, &yes, INT_32);

#ifdef WIN32
  ioctlsocket(server->socket, FIONBIO, (u_long*)&yes);
#else
  fcntl(server->socket, F_SETFL, FLAGS|O_NONBLOCK);
#endif

  // connect
  status = connect(server->socket, sa, sn);
  if (status < 0) {
#ifdef WIN32
    errno = WSAGetLastError();
    if (errno != WSAEINPROGRESS && errno != WSAEWOULDBLOCK)
#else
    if (errno != EINPROGRESS)
#endif
    {
      if (errmsg) {
        ZVAL_STRING(errmsg, strerror(errno), 1);
      }
      mongo_util_disconnect(server);
      return FAILURE;
    }

    while (1) {
      fd_set rset, wset, eset;

      FD_ZERO(&rset);
      FD_SET(server->socket, &rset);
      FD_ZERO(&wset);
      FD_SET(server->socket, &wset);
      FD_ZERO(&eset);
      FD_SET(server->socket, &eset);

      if (select(server->socket+1, &rset, &wset, &eset, &tval) == 0) {
        if (errmsg) {
          ZVAL_STRING(errmsg, strerror(errno), 1);
        }
        mongo_util_disconnect(server);
        return FAILURE;
      }

      // if our descriptor has an error
      if (FD_ISSET(server->socket, &eset)) {
        if (errmsg) {
          ZVAL_STRING(errmsg, strerror(errno), 1);
        }
        mongo_util_disconnect(server);
        return FAILURE;
      }

      // if our descriptor is ready break out
      if (FD_ISSET(server->socket, &wset) || FD_ISSET(server->socket, &rset)) {
        break;
      }
    }

    size = sn;

    connected = getpeername(server->socket, sa, &size);
    if (connected == FAILURE) {
      if (errmsg) {
        ZVAL_STRING(errmsg, strerror(errno), 1);
      }
      mongo_util_disconnect(server);
      return FAILURE;
    }

    // set connected
    server->connected = 1;
  }
  else if (status == SUCCESS) {
    server->connected = 1;
  }


// reset flags
#ifdef WIN32
  ioctlsocket(server->socket, FIONBIO, &no);
#else
  fcntl(server->socket, F_SETFL, FLAGS);
#endif
  return SUCCESS;
}

int mongo_util_connect_authenticate(mongo_server *server, zval *errmsg TSRMLS_DC) {
  zval *connection, *db, *db_name, *username, *password, *ok;
  int logged_in = 0;
  mongo_link *temp_link;

  // if we're not using authentication, we're always logged in
  if (!server->username || !server->password) {
    return SUCCESS;
  }

  // make a "fake" connection
  MAKE_STD_ZVAL(connection);
  object_init_ex(connection, mongo_ce_Mongo);
  temp_link = (mongo_link*)zend_object_store_get_object(connection TSRMLS_CC);
  temp_link->server_set = (mongo_server_set*)emalloc(sizeof(mongo_server_set));
  temp_link->server_set->num = 1;
  temp_link->server_set->ts = 0;
  temp_link->server_set->server_ts = 0;
  temp_link->server_set->server = server;
  temp_link->server_set->master = temp_link->server_set->server;

  // initialize zval strings
  MAKE_STD_ZVAL(db_name);
  ZVAL_STRING(db_name, server->db, 1);
  MAKE_STD_ZVAL(username);
  ZVAL_STRING(username, server->username, 1);
  MAKE_STD_ZVAL(password);
  ZVAL_STRING(password, server->password, 1);

  // get admin db
  MAKE_STD_ZVAL(db);
  MONGO_METHOD1(Mongo, selectDB, db, connection, db_name);

  // log in
  MAKE_STD_ZVAL(ok);
  MONGO_METHOD2(MongoDB, authenticate, ok, db, username, password);

  if (EG(exception)) {
    zend_clear_exception(TSRMLS_C);

    zval_ptr_dtor(&db_name);
    zval_ptr_dtor(&db);
    zval_ptr_dtor(&username);
    zval_ptr_dtor(&password);

    // TODO: pick up error message
    if (errmsg) {
      ZVAL_STRING(errmsg, "failed running authenticate", 1);
    }
    return FAILURE;
  }

  zval_ptr_dtor(&db_name);
  zval_ptr_dtor(&db);
  zval_ptr_dtor(&username);
  zval_ptr_dtor(&password);

  // reset the socket so we don't close it when this is dtored
  temp_link->server_set->server = 0;
  efree(temp_link->server_set);
  temp_link->server_set = 0;
  zval_ptr_dtor(&connection);

  if (EG(exception)) {
    zval_ptr_dtor(&ok);
    return FAILURE;
  }

  if (Z_TYPE_P(ok) == IS_ARRAY) {
    zval **status;
    if (zend_hash_find(HASH_P(ok), "ok", strlen("ok")+1, (void**)&status) == SUCCESS) {
      logged_in = (Z_TYPE_PP(status) == IS_BOOL && Z_BVAL_PP(status)) || Z_DVAL_PP(status) == 1;
    }
  }
  else {
    logged_in = Z_BVAL_P(ok);
  }

  // check if we've logged in successfully
  if (!logged_in) {
    char *full_error;
    spprintf(&full_error, 0, "Couldn't authenticate with database %s: username [%s], password [%s]", server->db, server->username, server->password);
    if (errmsg) {
      ZVAL_STRING(errmsg, full_error, 0);
    }
    zval_ptr_dtor(&ok);
    return FAILURE;
  }

  // successfully logged in
  zval_ptr_dtor(&ok);
  return SUCCESS;
}



int mongo_util_connect__sockaddr(struct sockaddr *sa, int family, char *host, int port, zval *errmsg) {
#ifndef WIN32
  if (family == AF_UNIX) {
    struct sockaddr_un* su = (struct sockaddr_un*)(sa);
    su->sun_family = AF_UNIX;
    strncpy(su->sun_path, host, sizeof(su->sun_path));
  } else {
#endif
    struct hostent *hostinfo;
    struct sockaddr_in* si = (struct sockaddr_in*)(sa);

    si->sin_family = AF_INET;
    si->sin_port = htons(port);
    hostinfo = (struct hostent*)gethostbyname(host);

    if (hostinfo == NULL) {
      if (errmsg) {
        char *errstr;
        spprintf(&errstr, 0, "couldn't get host info for %s", host);
        ZVAL_STRING(errmsg, errstr, 0);
      }
      return FAILURE;
    }

#ifndef WIN32
    si->sin_addr = *((struct in_addr*)hostinfo->h_addr);
  }
#else
    si->sin_addr.s_addr = ((struct in_addr*)(hostinfo->h_addr))->s_addr;
#endif

  return SUCCESS;
}


int mongo_util_disconnect(mongo_server *server) {
  pid_t pid;

  if (!server || !server->socket) {
    return 0;
  }

  pid = getpid();
  if (server->owner != pid) {
    return 0;
  }

  MONGO_UTIL_DISCONNECT(server->socket);
  server->connected = 0;
  server->socket = 0;

  return 1;
}

