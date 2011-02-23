// pool.c
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

#include <php.h>

int mongo_util_pool_get(mongo_server *server, time_t timeout, zval *errmsg TSRMLS_DC) {
  stack_container *monitor;
  stack_node *node;
  
  if ((monitor = get_monitor(server TSRMLS_CC)) == 0) {
    return 0;
  }
  
  node = monitor->top;
  
  // null <- [conn1]
  //          ^node
  if (node) {
    stack_node *target = node;
    server->connection = target->conn;
    
    // pop stack
    monitor->top = node->next;

    free(target);
    return SUCCESS;
  }
  // null
  else {
    // add this server to the list of monitored servers for this pool
    mongo_server *list = monitor->servers;
    server->next_in_pool = list;
    monitor->servers = server;
    
    // create a new connection, no point in adding to stack
    return create_connection(server, timeout, errmsg TSRMLS_CC);
  }
}

void mongo_util_pool_done(server_set* set) {
  stack_container *monitor;
  stack_node *node;

  if ((monitor = get_monitor(server TSRMLS_CC)) == 0) {
    // TODO: if we couldn't push this, close the connection
    
    return;
  }
  
  node = (stack_node*)malloc(sizeof(stack_node));
  node->conn = server->connection;
  node->next = monitor->top;
  monitor->top = node;
}

void mongo_util_pool_failed(mongo_server *server TSRMLS_CC) {
  stack_container *monitor;
  mongo_server *temp;
  
  if ((monitor = get_monitor(server TSRMLS_CC) == 0)) {
    return;
  }

  // close all open connections
  temp = monitor->servers;
  while (temp) {
    php_mongo_disconnect_server(temp);
    temp = temp->next;
  }

  // remove any connections from the stack
  while (top) {
    stack_node *current = top;
    top = top->next;

    free(current);
  }
}

HashTable *get_connection_pools() {
  zend_rsrc_list_entry *le = 0, le_struct;
  
  // get persistent connection hash
  if (zend_hash_find(&EG(persistent_list), "mongoConnectionPool",
                     sizeof("mongoConnectionPool"), (void**)&le) == FAILURE) {
    le = &le_struct;
    le->ptr = 0;

    // TODO: store in persistent list
  }
  if (!le->ptr) {
    le->ptr = (HashTable*)malloc(sizeof(HashTable));
    if (!le->ptr) {
      return 0;
    }

    zend_hash_init(le->ptr, 8, 0, mongo_util_hash_dtor, 1);
  }

  return le->ptr;
}

// get stack of this server's connection
stack_container *get_monitor(mongo_server *server TSRMLS_DC) {
  HashTable *pools;
  stack_monitor *monitor;
  char *id = get_id(server TSRMLS_CC);

  if ((pools = get_connection_pools()) == 0) {
    return 0;
  }
  
  if (zend_hash_find(pools, id, sizeof(id), (void**)&monitor) == FAILURE) {    
    monitor = (stack_node*)malloc(sizeof(stack_container));
    if (!monitor) {
      return 0;
    }
    
    monitor->top = 0;
    monitor->servers = 0;
    
    if (zend_hash_add(pools, hashed_id, sizeof(hashed_id), &monitor) == FAILURE) {
      free(monitor);
      return 0;
    }
  }
  
  return monitor;
}

char* mongo_util_pool__get_id(mongo_server *server TSRMLS_CC) {
  char *id;

  estrndup(&id, 0, "%s:%d.%s.%s.%s", server->host, server->port,
           server->db ? server->db : "",
           server->user ? server->user : "",
           server->password ? server->password : "");
  
  return id;
}

int mongo_util_pool__create_connection(mongo_server *server, time_t timeout, zval *errmsg TSRMLS_DC) {
  return (php_mongo_connect_nonb(server, timeout, errmsg) == SUCCESS) &&
    // authenticate, if necessary
    (php_mongo_do_authenticate(link, errmsg TSRMLS_CC) == SUCCESS);
}

int mongo_util_pool__connect_nonb(mongo_server *server, int timeout, zval *errmsg) {
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
    ZVAL_STRING(errmsg, "Could not create socket", 1);
    return FAILURE;
  }

#else
  struct sockaddr_un su;
  uint size;
  int yes = 1;

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
    ZVAL_STRING(errmsg, strerror(errno), 1);
    return FAILURE;
  }
#endif

  // timeout: set in ms or default of 20
  tval.tv_sec = timeout <= 0 ? 20 : timeout / 1000;
  tval.tv_usec = timeout <= 0 ? 0 : (timeout % 1000) * 1000;

  // get addresses
  if (php_mongo_get_sockaddr(sa, family, server->host, server->port, errmsg) == FAILURE) {
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
      ZVAL_STRING(errmsg, strerror(errno), 1);      
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
        ZVAL_STRING(errmsg, strerror(errno), 1);      
        return FAILURE;
      }

      // if our descriptor has an error
      if (FD_ISSET(server->socket, &eset)) {
        ZVAL_STRING(errmsg, strerror(errno), 1);      
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
      ZVAL_STRING(errmsg, strerror(errno), 1);
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

static int php_mongo_do_authenticate(mongo_server *server, zval *errmsg TSRMLS_DC) {
  zval *connection, *db, *db_name, *username, *password, *ok;
  int logged_in = 0;
  mongo_link *temp_link;
  mongo_server *temp_server;

  // if we're not using authentication, we're always logged in
  if (!server->username || !server->password) { 
    return SUCCESS;
  }

  // make a "fake" connection
  MAKE_STD_ZVAL(connection);
  object_init_ex(connection, mongo_ce_Mongo);
  temp_link = (mongo_link*)zend_object_store_get_object(connection TSRMLS_CC);
  temp_link->persist = link->persist;
  temp_link->server_set = (mongo_server_set*)emalloc(sizeof(mongo_server_set));
  temp_link->server_set->num = 1;
  temp_link->server_set->slaves = 0;
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

  zend_try {
    // get admin db
    MAKE_STD_ZVAL(db);
    MONGO_METHOD1(Mongo, selectDB, db, connection, db_name);
  
    // log in
    MAKE_STD_ZVAL(ok);
    MONGO_METHOD2(MongoDB, authenticate, ok, db, username, password);
  } zend_catch {
    zval_ptr_dtor(&db_name);
    zval_ptr_dtor(&db);
    zval_ptr_dtor(&username);
    zval_ptr_dtor(&password);

    // TODO: pick up error message
    ZVAL_STRING(errmsg, "failed running authenticate", 1);
    return FAILURE;
  } zend_end_try();
    
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
    spprintf(&full_error, 0, "Couldn't authenticate with database %s: username [%s], password [%s]", Z_STRVAL_P(link->db), Z_STRVAL_P(link->username), Z_STRVAL_P(link->password));
    ZVAL_STRING(errmsg, full_error, 0);
    zval_ptr_dtor(&ok);
    return FAILURE;
  }

  // successfully logged in
  zval_ptr_dtor(&ok);
  return SUCCESS;
}


static int php_mongo_get_sockaddr(struct sockaddr *sa, int family, char *host, int port, zval *errmsg) {
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
      char *errstr;
      spprintf(&errstr, 0, "couldn't get host info for %s", host); 
      ZVAL_STRING(errmsg, errstr, 0);
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


int php_mongo_disconnect_server(mongo_server *server) {
  if (!server || !server->connected) {
    return 0;
  }
  
  server->connected = 0;
#ifdef WIN32
  shutdown(server->socket, 2);
  closesocket(server->socket);
  WSACleanup();
#else
  close(server->socket);
#endif /* WIN32 */

  return 1;
}
