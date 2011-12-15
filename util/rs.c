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
#include "../bson.h"
#include "../mongo.h"
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

/**
 * Add hosts from ht to *hosts[pos] and increment the index.
 */
static void add_hosts(char **hosts, int *pos, zval **list);

/**
 * Creates a monitor.  For each server listed in link, creates a container
 * pointing to this monitor.  For example, if we have link with servers S1,S2,S3
 * and this creates monitor M, we'd end up with:
 *
 * "rsMonitor:S1" -> M -> (S3->S2->S1)
 * "rsMonitor:S2" -> M -> (S3->S2->S1)
 * "rsMonitor:S3" -> M -> (S3->S2->S1)
 */
static rs_monitor* initialize_new_monitor(mongo_link *link TSRMLS_DC);

/**
 * Created a rs_container instance to store a monitor under server's label.
 */
static rs_container* store_monitor(rs_monitor *monitor, mongo_server *server TSRMLS_DC);

/**
 * Add hosts, passives, and arbiters to **hosts.
 */
static void mongo_util_rs__populate_hosts(zval *response, char ***hosts, int *len TSRMLS_DC);

/**
 * Repopulate the link->server_set->servers list with the isMaster response.
 */
static void mongo_util_rs__repopulate(rs_monitor *monitor, zval *response TSRMLS_DC);

/**
 * Remove a host from the link->server_set->servers list.
 */
static void mongo_util_rs__remove_seed(rs_monitor *monitor, rsm_server *server TSRMLS_DC);

/**
 * Clean up a server that is removed from the link->server_set->servers list.
 */
static void mongo_util_rs__remove_bookkeeping(rs_monitor *monitor, rsm_server *server TSRMLS_DC);


int mongo_util_rs_init(mongo_link *link TSRMLS_DC) {
  rs_monitor *monitor;

  // getting a new monitor populates the struct with initial pings
  if ((monitor = mongo_util_rs__get_monitor(link TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  // get the primary, if possible, from the monitor
  if (monitor->primary) {

    link->server_set->master = mongo_util_server_copy(monitor->primary, link->server_set->master, NO_PERSIST TSRMLS_CC);

    // just to be explicit about it: we won't fetch a slave until one is requested
    link->slave = 0;
  }

  return SUCCESS;
}

zval* mongo_util_rs__cmd(char *cmd, mongo_server *current TSRMLS_DC) {
  zval *ismaster = 0, *result = 0;

  // query = { ismaster : 1 }
  // we cannot nest this list {query : {ismaster : 1}} because mongos is stupid
  MAKE_STD_ZVAL(ismaster);
  array_init(ismaster);
  add_assoc_long(ismaster, cmd, 1);

  result = mongo_db_cmd(current, ismaster TSRMLS_CC);

  zval_ptr_dtor(&ismaster);

  return result;
}

void mongo_util_rs_refresh(rs_monitor *monitor, time_t now TSRMLS_DC) {
  rsm_server *current;
  zval *good_response = 0;

  // refreshes host list
  if (now - monitor->last_ismaster < MONGO_ISMASTER_INTERVAL) {
    return;
  }

  monitor->last_ismaster = now;

  mongo_log(MONGO_LOG_RS, MONGO_LOG_INFO TSRMLS_CC, "%s: pinging at %d", monitor->name, now);

  // we are going clear the hosts list and repopulate
  current = monitor->servers;
  while(current && !good_response) {
    zval *response = mongo_util_rs__cmd("ismaster", current->server TSRMLS_CC);

    if (response && Z_TYPE_P(response) == IS_ARRAY) {
      zval **ok = 0;
      if (zend_hash_find(HASH_P(response), "ok", strlen("ok")+1, (void**)&ok) == SUCCESS &&
          Z_NUMVAL_PP(ok, 1)) {
        zval **name = 0;

        if (zend_hash_find(HASH_P(response), "setName", strlen("setName")+1, (void**)&name) == SUCCESS &&
            Z_TYPE_PP(name) == IS_STRING && strncmp(monitor->name, Z_STRVAL_PP(name), strlen(monitor->name)) != 0) {
          mongo_log(MONGO_LOG_RS, MONGO_LOG_WARNING TSRMLS_CC, "rs: given name %s does not match discovered name %s",
                    monitor->name, Z_STRVAL_PP(name));
        }

        good_response = response;
        break;
      }
      else {
        mongo_log(MONGO_LOG_RS, MONGO_LOG_INFO TSRMLS_CC, "rs: did not get a good isMaster response from %s",
                  current->server->label);

        zval_ptr_dtor(&response);
      }
    }
    current = current->next;
  }

  if (good_response) {
    mongo_util_rs__repopulate(monitor, good_response TSRMLS_CC);
    zval_ptr_dtor(&good_response);
  }
  else {
    mongo_log(MONGO_LOG_RS, MONGO_LOG_INFO TSRMLS_CC, "rs: did not get any isMaster responses, giving up");
  }
}

static void mongo_util_rs__repopulate(rs_monitor *monitor, zval *response TSRMLS_DC) {
  char **hosts = 0;
  int len = 0, i = 0;
  rsm_server *seed, *eo_list;

  mongo_util_rs__populate_hosts(response, &hosts, &len TSRMLS_CC);

  // clear primary
  monitor->primary = 0;

  // seed_list: A, B; hosts: X, Y, Z
  seed = monitor->servers;
  while (seed) {
    int found = 0;
    rsm_server *next;

    for (i=0; i<len; i++) {
      char *host = hosts[i];
      if (!host) {
        continue;
      }

      // if A is equal to or an alias for X:
      if (strncmp(seed->server->label, host, strlen(seed->server->label)) == 0 ||
          mongo_util_server_cmp(seed->server->label, host TSRMLS_CC) == 0) {
        // remove X
        // hosts[i] will be freed when good_response is freed in refresh()
        hosts[i] = 0;

        // continue to B
        found = 1;
        break;
      }
    }

    // seed is set to null in if(!found) block
    next = seed->next;

    // if seed does not match any host remove A from final list
    if (!found) {
      mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "rs: removing %s from host list", seed->server->label);
      mongo_util_rs__remove_seed(monitor, seed TSRMLS_CC);

      // now 'seed' is null
    }

    seed = next;
  }

  // get to the end of the list
  eo_list = monitor->servers;
  while (eo_list && eo_list->next) {
    eo_list = eo_list->next;
  }

  // iterate through remaining hosts that did not match any seed
  for (i=0; i<len; i++) {
    char *host = hosts[i];
    rsm_server *rsm;
    mongo_server *server;

    if (!host) {
      continue;
    }

    if (!(server = create_mongo_server_persist(&host, monitor TSRMLS_CC))) {
      continue;
    }

    // we need to call init here in case this is a new server name (without a timeout set)
    mongo_util_pool_refresh(server, MONGO_RS_TIMEOUT TSRMLS_CC);

    rsm = (rsm_server*)pemalloc(sizeof(rsm_server), 1);
    rsm->server = server;
    rsm->next = 0;

    mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "appending new host to list: %s", server->label);

    if (!eo_list) {
      monitor->servers = eo_list = rsm;
    }
    else {
      eo_list->next = rsm;
      eo_list = eo_list->next;
    }
  }

  // this array points to good_response elements, so we don't need to free its guts directly
  efree(hosts);
}

static void mongo_util_rs__remove_seed(rs_monitor *monitor, rsm_server *rsm TSRMLS_DC) {
  rsm_server *current = monitor->servers, *prev = 0;

  if (!rsm) {
    mongo_log(MONGO_LOG_RS, MONGO_LOG_WARNING TSRMLS_CC, "rs: trying to remove null seed");
    return;
  }

  if (!current) {
    mongo_log(MONGO_LOG_RS, MONGO_LOG_WARNING TSRMLS_CC, "rs: trying to remove %s from empty list",
              rsm->server->label);
    return;
  }

  if (current == rsm) {
    monitor->servers = current->next;
    mongo_util_rs__remove_bookkeeping(monitor, current TSRMLS_CC);
    return;
  }

  do {
    prev = current;
    current = current->next;

    if (current == rsm) {
      // if we have 1->2->3, this does 1->3 and then cleans up 2
      prev->next = current->next;
      mongo_util_rs__remove_bookkeeping(monitor, current TSRMLS_CC);
      return;
    }
  } while (current);

  mongo_log(MONGO_LOG_RS, MONGO_LOG_WARNING TSRMLS_CC, "rs: trying to remove %s from list, but could not find it",
            rsm->server->label);

  return;
}

static void mongo_util_rs__remove_bookkeeping(rs_monitor *monitor, rsm_server *server TSRMLS_DC) {
  // make sure primary isn't pointing to invalid mem
  if (monitor->primary == server->server) {
    monitor->primary = 0;
  }

  // this closes the connection and frees the memory
  php_mongo_server_free(server->server, PERSIST TSRMLS_CC);
  // free the container
  pefree(server, 1);
}

static void mongo_util_rs__populate_hosts(zval *response, char ***hosts, int *len TSRMLS_DC) {
  int total = 0, pos = 0;
  zval **hosts_z = 0, **passives = 0, **arbiters = 0;

  if (zend_hash_find(HASH_P(response), "hosts", strlen("hosts")+1, (void**)&hosts_z) == SUCCESS) {
    total += zend_hash_num_elements(HASH_PP(hosts_z));
  }
  if (zend_hash_find(HASH_P(response), "passives", strlen("passives")+1, (void**)&passives) == SUCCESS) {
    total += zend_hash_num_elements(HASH_PP(passives));
  }
  if (zend_hash_find(HASH_P(response), "arbiters", strlen("arbiters")+1, (void**)&arbiters) == SUCCESS) {
    total += zend_hash_num_elements(HASH_PP(arbiters));
  }

  (*len) = total;
  (*hosts) = (char**)emalloc(sizeof(char*)*(*len));

  add_hosts(*hosts, &pos, hosts_z);
  add_hosts(*hosts, &pos, passives);
  add_hosts(*hosts, &pos, arbiters);

  if (pos != *len) {
    mongo_log(MONGO_LOG_RS, MONGO_LOG_WARNING TSRMLS_CC, "rs: got two different lengths for isMaster hosts: %d vs. %d",
              pos, *len);
  }
}


static void add_hosts(char **hosts, int *pos, zval **list) {
  zval **data;
  HashPosition pointer;
  HashTable *ht;

  if (!list || !*list) {
    return;
  }

  ht = Z_ARRVAL_PP(list);

  for (zend_hash_internal_pointer_reset_ex(ht, &pointer);
       zend_hash_get_current_data_ex(ht, (void**) &data, &pointer) == SUCCESS;
       zend_hash_move_forward_ex(ht, &pointer)) {
    hosts[*pos] = Z_STRVAL_PP(data);
    (*pos)++;
  }
}


mongo_server* mongo_util_rs_get_master(mongo_link *link TSRMLS_DC) {
  rs_monitor *monitor;

  // if we're still connected to master, return it
  if (link->server_set->master && link->server_set->master->connected) {
    return link->server_set->master;
  }

  mongo_log(MONGO_LOG_RS, MONGO_LOG_FINE TSRMLS_CC, "%s: getting master", link->rs);

  if ((monitor = mongo_util_rs__get_monitor(link TSRMLS_CC)) == 0 ||
      monitor->primary == 0) {
    return 0;
  }

  link->server_set->master = mongo_util_server_copy(monitor->primary, link->server_set->master, NO_PERSIST TSRMLS_CC);
  return link->server_set->master;
}

rs_monitor* mongo_util_rs__get_monitor(mongo_link *link TSRMLS_DC) {
  mongo_server *current = link->server_set->server;

  // generate all possible ids
  while (current) {
    zend_rsrc_list_entry *le = 0;
    char id[256];

    mongo_buf_init(id);
    mongo_buf_append(id, MONGO_RS);
    mongo_buf_append(id, current->label);

    // see if any of them correspond to existing replica set monitors
    if (zend_hash_find(&EG(persistent_list), id, strlen(id)+1, (void**)&le) == SUCCESS) {
      return ((rs_container*)le->ptr)->monitor;
    }

    current = current->next;
  }

  // if not, initialize a new monitor
  return initialize_new_monitor(link TSRMLS_CC);
}

static rs_container* store_monitor(rs_monitor *monitor, mongo_server *server TSRMLS_DC) {
  zend_rsrc_list_entry nle;
  char id[256];
  rs_container *container;

  mongo_buf_init(id);
  mongo_buf_append(id, MONGO_RS);
  mongo_buf_append(id, server->label);
  mongo_log(MONGO_LOG_RS, MONGO_LOG_INFO TSRMLS_CC, "rs: adding a new monitor labeled %s\n", id);

  container = (rs_container*)pemalloc(sizeof(rs_container), 1);
  container->owner = 0;
  container->monitor = monitor;

  nle.ptr = container;
  nle.type = le_prs;
  nle.refcount = 1;
  zend_hash_add(&EG(persistent_list), id, strlen(id)+1, &nle, sizeof(zend_rsrc_list_entry), NULL);

  return container;
}

static rs_monitor* initialize_new_monitor(mongo_link *link TSRMLS_DC) {
  rs_container *container;
  rs_monitor *monitor;
  mongo_server *current;

  monitor = (rs_monitor*)pemalloc(sizeof(rs_monitor), 1);
  memset(monitor, 0, sizeof(rs_monitor));

  monitor->name = pestrdup(link->rs, 1);

  if (link->username && link->password && link->db) {
    monitor->username = pestrdup(link->username, 1);
    monitor->password = pestrdup(link->password, 1);
    monitor->db = pestrdup(link->db, 1);
  }

  current = link->server_set->server;
  while (current) {
    rsm_server *r_server;

    // add current to the rsm list
    r_server = (rsm_server*)pemalloc(sizeof(rsm_server), 1);
    r_server->next = 0;
    r_server->server = mongo_util_server_copy(current, 0, PERSIST TSRMLS_CC);

    if (monitor->servers) {
      r_server->next = monitor->servers;
    }
    monitor->servers = r_server;

    // create a container for current
    container = store_monitor(monitor, current TSRMLS_CC);

    current = current->next;
  }

  if (container) {
    container->owner = 1;
  }

  mongo_util_rs__ping(monitor TSRMLS_CC);

  return monitor;
}

void mongo_util_rs_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  rs_container *container;
  rs_monitor *monitor;
  rsm_server *current;
  int owns_monitor = 0;

  if (!rsrc || !rsrc->ptr) {
    return;
  }

  container = rsrc->ptr;
  monitor = container->monitor;

  owns_monitor = container->owner;

  pefree(container, 1);
  rsrc->ptr = 0;

  // if we aren't responsible for freeing the monitor, we're done
  if (!owns_monitor) {
    return;
  }

  current = monitor->servers;
  while (current) {
    rsm_server *prev = current;
    current = current->next;

    php_mongo_server_free(prev->server, PERSIST TSRMLS_CC);
    pefree(prev, 1);
  }

  pefree(monitor->name, 1);
  pefree(monitor->username, 1);
  pefree(monitor->password, 1);
  pefree(monitor->db, 1);
  pefree(monitor, 1);
}

void mongo_util_rs_ping(mongo_link *link TSRMLS_DC) {
  rs_monitor *monitor;

  if (!link->rs) {
    return;
  }

  // getting a new monitor populates the struct with initial pings
  if ((monitor = mongo_util_rs__get_monitor(link TSRMLS_CC)) == 0) {
    return;
  }

  if (time(0) - monitor->last_ismaster < MONGO_PING_INTERVAL) {
    return;
  }

  mongo_util_rs__ping(monitor TSRMLS_CC);
}

void mongo_util_rs__ping(rs_monitor *monitor TSRMLS_DC) {
  int now;
  rsm_server *current;

  now = time(0);

  mongo_util_rs_refresh(monitor, now TSRMLS_CC);

  current = monitor->servers;
  while (current) {
    // this pings the server and, if up, checks if it's primary
    if (mongo_util_server_ping(current->server, now TSRMLS_CC) == SUCCESS) {
      if (mongo_util_server_get_state(current->server TSRMLS_CC) == 1) {
        monitor->primary = current->server;
      }
    }
    else if (monitor->primary == current->server) {
      monitor->primary = 0;
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

int mongo_util_rs__set_slave(mongo_link *link, char **errmsg TSRMLS_DC) {
  rs_monitor *monitor;
  rsm_server *possible_slave;
  int min_bucket = INT_MAX, slave_count = 0, random_num;

  if (!link->rs || !link->server_set) {
    *(errmsg) = estrdup("Connection is not initialized or not a replica set");
    return FAILURE;
  }

  // getting a new monitor populates the struct with initial pings
  if ((monitor = mongo_util_rs__get_monitor(link TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  // If we have M members total with N members in bucket B0, we want a 1/N
  // chance of choosing any given member of B0.
  random_num = rand();

  possible_slave = monitor->servers;

  link->slave = 0;
  while (possible_slave) {
    int bucket;

    if (!mongo_util_server_get_readable(possible_slave->server TSRMLS_CC)) {
      possible_slave = possible_slave->next;
      continue;
    }

    bucket = mongo_util_server_get_bucket(possible_slave->server TSRMLS_CC);

    // first, handle if this member is in the same bucket as the currently chosen slave
    if (bucket == min_bucket && possible_slave->server != monitor->primary) {
      slave_count++;

      // 1/N chance of this slave being the chosen one
      if (random_num % slave_count == 0) {
        link->slave = mongo_util_server_copy(possible_slave->server, link->slave, NO_PERSIST TSRMLS_CC);
      }
    }

    // if this server is in a smaller bucket than the current min, use it
    if (bucket < min_bucket && possible_slave->server != monitor->primary) {
      link->slave = mongo_util_server_copy(possible_slave->server, link->slave, NO_PERSIST TSRMLS_CC);
      min_bucket = bucket;

      // reset slave count - there's only one slave we've seen so far in this bucket
      slave_count = 1;
    }

    possible_slave = possible_slave->next;
  }

  if (link->slave) {
    return RS_SECONDARY;
  }
  // if we've run out of possibilities, use the master
  else if (!link->slave && monitor->primary) {
    link->slave = mongo_util_server_copy(monitor->primary, link->slave, NO_PERSIST TSRMLS_CC);
    return RS_PRIMARY;
  }

  *errmsg = estrdup("No secondary found");
  return FAILURE;
}
