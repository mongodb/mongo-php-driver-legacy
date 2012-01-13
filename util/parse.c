// parse.c
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

#include "../php_mongo.h"
#include "rs.h"
#include "parse.h"
#include "log.h"

static char* php_mongo_get_host(char** current, int domain_socket, int persist);
static int php_mongo_get_port(char**);
static mongo_server* create_mongo_server(char **current, mongo_link *link TSRMLS_DC);
static mongo_server* _create_mongo_server(char **current, int persist TSRMLS_DC);

ZEND_EXTERN_MODULE_GLOBALS(mongo);

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_ConnectionException;

static mongo_server* create_mongo_server(char **current, mongo_link *link TSRMLS_DC) {
  return _create_mongo_server(current, NO_PERSIST TSRMLS_CC);
  // auth info is copied in php_mongo_parse_server
}

mongo_server* create_mongo_server_persist(char **current, rs_monitor *monitor TSRMLS_DC) {
  mongo_server *server;
  server = _create_mongo_server(current, PERSIST TSRMLS_CC);

  if (!server) {
    return 0;
  }

  if (monitor->username) {
    server->username = pestrdup(monitor->username, PERSIST);
  }
  if (monitor->password) {
    server->password = pestrdup(monitor->password, PERSIST);
  }
  if (monitor->db) {
    server->db = pestrdup(monitor->db, PERSIST);
  }

  return server;
}

static mongo_server* _create_mongo_server(char **current, int persist TSRMLS_DC) {
  char *host;
  int port;
  mongo_server *server;
  int domain_socket = 0;

  // localhost:1234
  // localhost,
  // localhost
  // /tmp/mongodb-port.sock
  // ^
  if (**current == '/') {
    domain_socket = 1;
  }

  if ((host = php_mongo_get_host(current, domain_socket, persist)) == 0) {
    return 0;
  }

  // localhost:27017
  //          ^
  if (domain_socket) {
    port = 0;
    if (**current == ':') {
      (*current)++;
      while (**current >= '0' && **current <= '9') {
        (*current)++;
      }
    }
  }
  else if ((port = php_mongo_get_port(current)) < 0) {
    pefree(host, persist);
    return 0;
  }

  // create a struct for this server
  server = (mongo_server*)pemalloc(sizeof(mongo_server), persist);
  memset(server, 0, sizeof(mongo_server));

  server->host = host;
  server->port = port;
  spprintf(&server->label, 0, "%s:%d", host, port);

  if (persist) {
    char *temp = server->label;
    server->label = (char*)pemalloc(strlen(temp)+1, persist);
    memcpy(server->label, temp, strlen(temp)+1);
    efree(temp);
  }

  return server;
}

/*
 * this deals with the new mongo connection format:
 * mongodb://username:password@host:port,host:port
 *
 * throws exception
 */
int php_mongo_parse_server(zval *this_ptr TSRMLS_DC) {
  zval *hosts_z;
  char *hosts, *current;
  mongo_link *link;
  mongo_server *current_server;

  mongo_log(MONGO_LOG_PARSE, MONGO_LOG_FINE TSRMLS_CC, "parsing servers");

  hosts_z = zend_read_property(mongo_ce_Mongo, getThis(), "server", strlen("server"), NOISY TSRMLS_CC);
  hosts = Z_STRLEN_P(hosts_z) ? Z_STRVAL_P(hosts_z) : 0;
  current = hosts;

  link = (mongo_link*)zend_object_store_get_object(getThis() TSRMLS_CC);

  // go with the default setup:
  // one connection to localhost:27017
  if (!hosts) {
    // set the top-level server set fields
    link->server_set = (mongo_server_set*)emalloc(sizeof(mongo_server_set));
    link->server_set->num = 1;
    link->server_set->ts = 0;
    link->server_set->server_ts = 0;

    // allocate one server
    link->server_set->server = (mongo_server*)emalloc(sizeof(mongo_server));
    memset(link->server_set->server, 0, sizeof(mongo_server));

    link->server_set->server->host = estrdup(MonGlo(default_host));
    link->server_set->server->port = MonGlo(default_port);
    spprintf(&link->server_set->server->label, 0, "%s:%d", MonGlo(default_host), MonGlo(default_port));
    link->server_set->master = link->server_set->server;

    return SUCCESS;
  }

  // check if it has the right prefix for a mongo connection url
  if (strstr(hosts, "mongodb://") == hosts) {
    char *at, *colon;

    // mongodb://user:pass@host:port,host:port
    //           ^
    current += 10;

    // mongodb://user:pass@host:port,host:port
    //                    ^
    at = strchr(current, '@');

    // mongodb://user:pass@host:port,host:port
    //               ^
    colon = strchr(current, ':');

    // check for username:password
    if (at && colon && at - colon > 0) {
      if (!link->username) {
        link->username = estrndup(current, colon-current);
      }
      if (!link->password) {
        link->password = estrndup(colon+1, at-(colon+1));
      }

      // move current
      // mongodb://user:pass@host:port,host:port
      //                     ^
      current = at+1;
    }
  }

  // now we're doing the same thing, regardless of prefix
  // host1[:27017][,host2[:27017]]+

  // allocate the server ptr
  link->server_set = (mongo_server_set*)emalloc(sizeof(mongo_server_set));
  link->server_set->ts = 0;
  link->server_set->server_ts = 0;

  // allocate the top-level server set fields
  link->server_set->num = 0;
  link->server_set->master = 0;

  // set server to 0 in case something goes wrong, then it won't be freed
  link->server_set->server = current_server = 0;

  // current is now pointing at the first server name

  // normal hostname
  while (*current) {
    mongo_server *server;
    char **current_ptr = &current;

    mongo_log(MONGO_LOG_PARSE, MONGO_LOG_FINE TSRMLS_CC, "current: %s", current);

    // method throws exception
    if (!(server = create_mongo_server(current_ptr, link TSRMLS_CC))) {
      zend_throw_exception(mongo_ce_ConnectionException, "couldn't parse connection string", 10 TSRMLS_CC);
      return FAILURE;
    }
    current = *current_ptr;

    link->server_set->num++;

    // initialize server list
    if (link->server_set->server == 0) {
      link->server_set->server = server;
      current_server = link->server_set->server;
    }
    // add a server
    else {
      current_server->next = server;
      current_server = current_server->next;
    }

    // localhost/dbname
    //          ^
    if (*current == '/') {
      break;
    }
    // localhost,
    // localhost
    //          ^
    if (*current == ',') {
      current++;
      while (*current == ' ') {
        current++;
      }
    }
  }

  // if this isn't the (invalid) form "host:port/"
  if (*current == '/' && *(current+1) != '\0') {
    current++;
    if (!link->db) {
      link->db = estrdup(current);
    }
  }

  // if we need to authenticate but weren't given a database, assume admin
  if (link->username && link->password) {
    mongo_server *c;

    if (!link->db) {
      link->db = estrdup("admin");
    }

    c = link->server_set->server;
    while (c) {
      c->db = estrdup(link->db);
      c->username = estrdup(link->username);
      c->password = estrdup(link->password);
      c = c->next;
    }
  }

  mongo_log(MONGO_LOG_PARSE, MONGO_LOG_FINE TSRMLS_CC, "done parsing");

  return SUCCESS;
}

// get the next host from the server string
static char* php_mongo_get_host(char **ip, int domain_socket, int persist) {
  char *end = *ip, *retval;

  // pick whichever exists and is sooner: ':', ',', '/', or '\0'
  while (*end && *end != ',' && *end != ':' && (*end != '/' || domain_socket)) {
    end++;
  }

  // sanity check
  if (end - *ip > 1 && end - *ip < 256) {
    int len = end-*ip;

    // return a copy
    retval = estrndup(*ip, len);
    if (persist) {
      char *temp = retval;
      retval = pestrdup(temp, persist);
      efree(temp);
    }

    // move to the end of this section of string
    *(ip) = end;

    return retval;
  }
  else {
    // you get nothing
    return 0;
  }

  // otherwise, this is the last thing in the string
  retval = pestrdup(*ip, persist);

  // move to the end of this string
  *(ip) = *ip + strlen(*ip);

  return retval;
}

// get the next port from the server string
static int php_mongo_get_port(char **ip) {
  char *end;
  int retval;

  // there might not even be a port
  if (**ip != ':') {
    return 27017;
  }

  // if there is, move past the colon
  // localhost:27017
  //          ^
  (*ip)++;

  end = *ip;
  // make sure the port is actually a number
  while (*end >= '0' && *end <= '9') {
    end++;
  }

  if (end == *ip) {
    return -1;
  }

  // this just takes the first numeric characters
  retval = atoi(*ip);

  // move past the port
  *(ip) += (end - *ip);

  return retval;
}
