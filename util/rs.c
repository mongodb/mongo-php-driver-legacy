// rs.c
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

#include "../php_mongo.h"
#include "../cursor.h"
#include "../db.h"

#include "log.h"
#include "rs.h"
#include "hash.h"
#include "pool.h"
#include "server.h"
#include "parse.h"

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_DB,
  *mongo_ce_Cursor;

extern int le_prs;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

mongo_server* mongo_util_rs__find_or_make_server(char *host, mongo_link *link TSRMLS_DC) {
  mongo_server *target_server, *eo_list = 0, *server;

  target_server = link->server_set->server;
  while (target_server) {
    // if we've found the "host" server, we're done
    if (strcmp(host, target_server->label) == 0) {
      return target_server;
    }
    eo_list = target_server;
    target_server = target_server->next;
  }

  // otherwise, create a new server from the host
  if (!(server = create_mongo_server(&host, host, link TSRMLS_CC))) {
    return 0;
  }

  // we need to call init here in case this is a new server name (without a timeout set)
  mongo_util_pool_refresh(server, link->timeout TSRMLS_CC);

  mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "appending to list: %s", server->label);

  // get to the end of the server list
  if (!link->server_set->server) {
    link->server_set->server = server;
  }
  else {
    if (eo_list && eo_list->next) {
      while (eo_list->next) {
        eo_list = eo_list->next;
      }
    }

    eo_list->next = server;
  }
  link->server_set->num++;
  return server;
}

void mongo_util_rs__repopulate_hosts(zval **hosts, mongo_link *link TSRMLS_DC) {
  zval **data;
  HashTable *hash;
  HashPosition pointer;

  hash = Z_ARRVAL_PP(hosts);
  for (zend_hash_internal_pointer_reset_ex(hash, &pointer);
       zend_hash_get_current_data_ex(hash, (void**) &data, &pointer) == SUCCESS;
       zend_hash_move_forward_ex(hash, &pointer)) {
    char *host = Z_STRVAL_PP(data);

    // this could fail if host is invalid, but it's okay if it does
    mongo_util_rs__find_or_make_server(host, link TSRMLS_CC);
  }
}

zval* mongo_util_rs__ismaster(mongo_server *current TSRMLS_DC) {
  zval *ismaster = 0, *result = 0;

  // query = { ismaster : 1 }
  // we cannot nest this list {query : {ismaster : 1}} because mongos is stupid
  MAKE_STD_ZVAL(ismaster);
  array_init(ismaster);
  add_assoc_long(ismaster, "ismaster", 1);

  result = mongo_db_cmd(current, ismaster TSRMLS_CC);

  zval_ptr_dtor(&ismaster);

  return result;
}

void mongo_util_rs_refresh(mongo_link *link, time_t now TSRMLS_DC) {
  mongo_server *current;
  zval **hosts, *good_response = 0;
  time_t *last_ping = 0;

  if (!link->rs) {
    return;
  }

  last_ping = mongo_util_rs__get_ping(link TSRMLS_CC);
  if (*last_ping + MONGO_PING_INTERVAL > now) {
    return;
  }
  (*last_ping) = now;

  mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "%s: pinging at %d", link->rs, now);

  // we are going clear the hosts list and repopulate
  current = link->server_set->server;
  while(current && !good_response) {
    zval *response = mongo_util_rs__ismaster(current TSRMLS_CC);

    if (response) {
      zval **ok = 0;
      if (zend_hash_find(HASH_P(response), "ok", strlen("ok")+1, (void**)&ok) == SUCCESS &&
          Z_NUMVAL_PP(ok, 1)) {
        zval **name = 0;

        if (zend_hash_find(HASH_P(response), "setName", strlen("setName")+1, (void**)&name) == SUCCESS &&
            Z_TYPE_PP(name) == IS_STRING && strncmp(link->rs, Z_STRVAL_PP(name), strlen(link->rs)) != 0) {
          mongo_log(MONGO_LOG_RS, MONGO_LOG_WARNING TSRMLS_CC, "rs: given name %s does not match discovered name %s",
                    link->rs, Z_STRVAL_PP(name));
        }

        good_response = response;
        break;
      }
      else {
        mongo_log(MONGO_LOG_RS, MONGO_LOG_INFO TSRMLS_CC, "rs: did not get a good isMaster response from %s",
                  current->label);

        zval_ptr_dtor(&response);
      }
    }
    current = current->next;
  }

  if (!good_response) {
    // we'll have to try again later
    return;
  }

  current = link->server_set->server;
  while (current) {
    mongo_server *prev = current;
    current = current->next;
    mongo_util_pool_done(prev TSRMLS_CC);
    efree(prev);
  }

  link->server_set->num = 0;
  link->server_set->server = 0;
  link->server_set->master = 0;
  link->slave = 0;

  mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "parsing replica set\n");

  if (good_response) {
    // repopulate
    if (zend_hash_find(HASH_P(good_response), "hosts", strlen("hosts")+1, (void**)&hosts) == SUCCESS) {
      mongo_util_rs__repopulate_hosts(hosts, link TSRMLS_CC);
    }
    if (zend_hash_find(HASH_P(good_response), "passives", strlen("passives")+1, (void**)&hosts) == SUCCESS) {
      mongo_util_rs__repopulate_hosts(hosts, link TSRMLS_CC);
    }
    if (zend_hash_find(HASH_P(good_response), "arbiters", strlen("arbiters")+1, (void**)&hosts) == SUCCESS) {
      mongo_util_rs__repopulate_hosts(hosts, link TSRMLS_CC);
    }
  }
}

mongo_server* mongo_util_rs_get_master(mongo_link *link TSRMLS_DC) {
  mongo_server *current;

  // for a single connection, return it
  if (!link->rs && link->server_set->num == 1) {
    if (link->server_set->server->connected) {
      return link->server_set->server;
    }
    return 0;
  }

  // if we're still connected to master, return it
  if (link->server_set->master && link->server_set->master->connected) {
    return link->server_set->master;
  }

  mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "%s: getting master", link->rs);

  // if this is a replica set, check the members occasionally and start the
  // iteration over again
  mongo_util_rs_ping(link TSRMLS_CC);

  // redetermine master

  current = link->server_set->server;
  while (current) {
    zval *response;
    int ismaster = 0;

    response = mongo_util_rs__ismaster(current TSRMLS_CC);
    if (!response) {
      current = current->next;
      continue;
    }
    ismaster = mongo_util_rs__get_ismaster(response TSRMLS_CC);

    if (ismaster) {
      link->server_set->master = current;
    }

    if (ismaster || mongo_util_rs__another_master(response, link TSRMLS_CC) == SUCCESS) {
      zval_ptr_dtor(&response);
      return link->server_set->master;
    }

    // reset response
    zval_ptr_dtor(&response);
    current = current->next;
  }

  return 0;
}

time_t* mongo_util_rs__get_ping(mongo_link *link TSRMLS_DC) {
  zend_rsrc_list_entry *le = 0;
  char *id;

  id = (char*)emalloc(strlen(link->rs)+strlen(MONGO_RS)+2);
  memcpy(id, MONGO_RS, strlen(MONGO_RS));
  memcpy(id+strlen(MONGO_RS), ":", 1);
  memcpy(id+strlen(MONGO_RS)+1, link->rs, strlen(link->rs));
  id[strlen(MONGO_RS)+strlen(link->rs)+1] = 0;

  if (zend_hash_find(&EG(persistent_list), id, strlen(id)+1, (void**)&le) == FAILURE) {
    zend_rsrc_list_entry nle;
    time_t *t;

    t = (time_t*)pemalloc(sizeof(time_t), 1);
    memset(t, 0, sizeof(time_t));

    // registering this links it to the dtor (mongo_util_pool_shutdown) so that
    // it can be auto-cleaned-up on shutdown
    nle.ptr = t;
    nle.type = le_prs;
    nle.refcount = 1;
    zend_hash_add(&EG(persistent_list), id, strlen(id)+1, &nle, sizeof(zend_rsrc_list_entry), NULL);

    efree(id);
    return t;
  }

  efree(id);
  return le->ptr;
}

void mongo_util_rs_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  if (!rsrc || !rsrc->ptr) {
    return;
  }

  pefree(rsrc->ptr, 1);
  rsrc->ptr = 0;
}

void mongo_util_rs_ping(mongo_link *link TSRMLS_DC) {
  int now;
  mongo_server *current;

  if (!link->rs) {
    return;
  }

  now = time(0);

  mongo_util_rs_refresh(link, now TSRMLS_CC);

  if (link->server_set->master == link->slave) {
    link->slave = 0;
  }

  // find a server with a hosts list
  current = link->server_set->server;
  while (current) {
    if (mongo_util_server_ping(current, now TSRMLS_CC) == SUCCESS) {
      link->server_set->master = current;
    }

    current = current->next;
  }
}

int mongo_util_rs__get_ismaster(zval *response TSRMLS_DC) {
  zval **ans;

  if (zend_hash_find(HASH_P(response), "ismaster", 9, (void**)&ans) == SUCCESS) {
    // in 1.6.*, this was a float
    if (Z_TYPE_PP(ans) == IS_DOUBLE) {
      return (Z_DVAL_PP(ans) == 1.0);
    }
    // in 1.7, it became a boolean
    else {
      return Z_BVAL_PP(ans);
    }
  }

  return 0;
}

int mongo_util_rs__another_master(zval *response, mongo_link *link TSRMLS_DC) {
  zval **primary;
  char *host;
  mongo_server *server;

  if (zend_hash_find(HASH_P(response), "primary", strlen("primary")+1, (void**)&primary) == FAILURE) {
    // this node can't reach the master, try someone else
    return FAILURE;
  }

  host = Z_STRVAL_PP(primary);
  if (!(server = mongo_util_rs__find_or_make_server(host, link TSRMLS_CC))) {
    return FAILURE;
  }

  if (mongo_util_pool_refresh(server, link->timeout TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "rs: connected to %s:%d\n", server->host, server->port);

  // if successful, we're connected to the master
  link->server_set->master = server;
  return SUCCESS;
}

int mongo_util_rs__set_slave(mongo_link *link, char **errmsg TSRMLS_DC) {
  mongo_server *possible_slave;
  int min_ping = INT_MAX;

  if (!link->rs || !link->server_set) {
    *(errmsg) = estrdup("Connection is not initialized or not a replica set");
    return FAILURE;
  }

  possible_slave = link->server_set->server;

  link->slave = 0;
  while (possible_slave) {
    int ping;

    if (!mongo_util_server_get_readable(possible_slave TSRMLS_CC)) {
      possible_slave = possible_slave->next;
      continue;
    }

    ping = mongo_util_server_get_ping_time(possible_slave TSRMLS_CC);
    if (ping < min_ping && possible_slave != link->server_set->master) {
      link->slave = possible_slave;
      min_ping = ping;
    }

    possible_slave = possible_slave->next;
  }

  if (link->slave) {
    return RS_SECONDARY;
  }
  // if we've run out of possibilities, use the master
  else if (!link->slave &&
      link->server_set->master && link->server_set->master->connected) {
    link->slave = link->server_set->master;
    return RS_PRIMARY;
  }

  *errmsg = estrdup("No secondary found");
  return FAILURE;
}
/*
int mongo_util_rs__get(char *id, mongo_link *link TSRMLS_DC) {
  rs_monitor *rs = get_rs_by_name(id);

  link->master = rs->primary;
  link->slave = get_slave(rs);
}
*/
