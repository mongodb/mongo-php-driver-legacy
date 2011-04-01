// link.c
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

#include "php_mongo.h"
#include "rs.h"
#include "hash.h"

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_DB,
  *mongo_ce_Cursor;
ZEND_EXTERN_MODULE_GLOBALS(mongo);

static mongo_server* find_or_make_server(char *host, mongo_link *link TSRMLS_DC);
static zval* create_fake_cursor(mongo_link *link TSRMLS_DC);

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

#ifdef DEBUG_CONN
  log1("appending to list: %s", server->label);
#endif

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
 
  // add this to the hosts list
  if (link->rs && link->server_set->hosts) {
    zval *null_p;
    
    null_p = (zval*)malloc(sizeof(zval));
    INIT_PZVAL(null_p);
    Z_TYPE_P(null_p) = IS_NULL;
    
    zend_hash_add(link->server_set->hosts, server->label, strlen(server->label)+1,
                  &null_p, sizeof(zval*), NULL);
  }
  
  return server;
}


zval* mongo_util_rs__create_fake_cursor(mongo_link *link TSRMLS_DC) {
  zval *cursor_zval, *query, *is_master;
  mongo_cursor *cursor;
  
  MAKE_STD_ZVAL(cursor_zval);
  object_init_ex(cursor_zval, mongo_ce_Cursor);

  // query = { ismaster : 1 }
  MAKE_STD_ZVAL(query);
  array_init(query);
  add_assoc_long(query, "ismaster", 1);

  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);

  // admin.$cmd.findOne({ query : { ismaster : 1 } })
  cursor->ns = estrdup("admin.$cmd");
  cursor->query = query;
  cursor->fields = 0;
  cursor->limit = -1;
  cursor->skip = 0;
  cursor->opts = 0;
  cursor->current = 0;
  cursor->timeout = 0;

  return cursor_zval;
}

zval* mongo_util_rs_ismaster(mongo_server *current TSRMLS_DC) {
  zval temp_ret, *errmsg, *response, *cursor_zval;
  mongo_link temp;
  mongo_server *temp_next = 0;
  mongo_server_set temp_server_set;
  mongo_cursor *cursor = 0;
  int exception = 0;
  
  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  // make a fake link
  temp.server_set = &temp_server_set;
  temp.server_set->num = 1;
  temp.server_set->server = current;
  temp.server_set->master = current;
  temp.rs = 0;

  // create a cursor
  cursor_zval = mongo_util_rs__create_fake_cursor(&temp TSRMLS_CC);
  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);
  
  // skip anything we're not connected to
  if (!current->connected && FAILURE == mongo_util_pool_get(current, errmsg)) {
#ifdef DEBUG_CONN
    log2("[c:php_mongo_get_master] not connected to %s:%d\n", current->host, current->port);
#endif
    zval_ptr_dtor(&errmsg);
    
    cursor->link = 0;
    zval_ptr_dtor(&cursor_zval);
    return 0;
  }
  zval_ptr_dtor(&errmsg);

  temp_next = current->next;
  current->next = 0;
  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);
  cursor->link = &temp;
   
  // need to call this after setting cursor->link
  // reset checks that cursor->link != 0
  MONGO_METHOD(MongoCursor, reset, &temp_ret, cursor_zval);
    
  MAKE_STD_ZVAL(response);
  ZVAL_NULL(response);

  zend_try {
    MONGO_METHOD(MongoCursor, getNext, response, cursor_zval);
  } zend_catch {
    exception = 1;
  } zend_end_try();

  current->next = temp_next;
  cursor->link = 0;
  zval_ptr_dtor(&cursor_zval);

  if (exception || IS_SCALAR_P(response)) {
    return 0;
  }

  return response;
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

void mongo_util_rs__refresh_list(mongo_link *link, zval *response TSRMLS_DC) {
  mongo_server *cur;
  zval **hosts;
  
  // we are going clear the hosts list and repopulate
  cur = link->server_set->server;
  while(cur) {
    mongo_server *prev = cur;
    cur = cur->next;
    mongo_util_pool_done(prev TSRMLS_CC);
  }
  link->server_set->server = 0;
  link->server_set->num = 0;
      
#ifdef DEBUG_CONN
  log0("parsing replica set\n");
#endif
        
  // repopulate
  if (zend_hash_find(HASH_P(response), "hosts", strlen("hosts")+1, (void**)&hosts) == SUCCESS) {
    mongo_util_rs__repopulate_hosts(hosts, link TSRMLS_CC);
  }
  if (zend_hash_find(HASH_P(response), "passives", strlen("passives")+1, (void**)&hosts) == SUCCESS) {
    mongo_util_rs__repopulate_hosts(hosts, link TSRMLS_CC);
  }    
}

// TODO: we don't actually need to find master on initial connection
mongo_server* mongo_util_rs_get_master(mongo_link *link TSRMLS_DC) {
  mongo_server *current;

#ifdef DEBUG_CONN
  log2("[c:php_mongo_get_master] servers: %d, rs? %d", link->server_set->num, link->rs);
#endif

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

  // if this is a replica set, check the members occasionally and start the
  // iteration over again
  mongo_util_rs_refresh(link TSRMLS_CC);
  
  // redetermine master

  current = link->server_set->server;
  while (current) {
    zval *response;
    int ismaster = 0;

    response = mongo_util_rs_ismaster(current TSRMLS_CC);
    if (!response) {
      current = current->next;
      continue;
    }
    ismaster = mongo_util_rs__get_ismaster(response);
        
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

void mongo_util_rs_refresh(mongo_link *link TSRMLS_DC) {
  int now;
  zval *response;
  mongo_server *current;
  
  now = time(0);
  if (!link->rs || link->server_set->server_ts + 5 >= now) {
    return;
  }

  // find a server with a hosts list
  current = link->server_set->server;
  while (current) {
    response = mongo_util_rs_ismaster(current TSRMLS_CC);
    
    if (response) {
      zval **hosts;
      if (zend_hash_find(HASH_P(response), "hosts", strlen("hosts")+1, (void**)&hosts) == SUCCESS ||
          zend_hash_find(HASH_P(response), "passives", strlen("passives")+1, (void**)&hosts) == SUCCESS) {
        break;
      }
      zval_ptr_dtor(&response);
    }

    current = current->next;
  }

  // if no one had a list of hosts, we can't do anything
  if (!current) {
    zval_ptr_dtor(&response);
    return;
  }

  // if someone did, refresh the list
  link->server_set->server_ts = now;
  mongo_util_rs__refresh_list(link, response TSRMLS_CC);
  zval_ptr_dtor(&response);  
}

void mongo_util_rs_ping(mongo_link *link TSRMLS_DC) {
  int now;
  
  now = time(0);
  
  if (link->server_set && link->server_set->ts + 5 < now) {
    zval *fake_zval;
    mongo_link *fake_link;
    char *errmsg, **errmsg_ptr;
    
    link->server_set->ts = now;
    
    MAKE_STD_ZVAL(fake_zval);
    object_init_ex(fake_zval, mongo_ce_Mongo);
    fake_link = (mongo_link*)zend_object_store_get_object(fake_zval TSRMLS_CC);
    fake_link->server_set = link->server_set;

    errmsg_ptr = &errmsg;
    get_heartbeats(fake_zval, errmsg_ptr TSRMLS_CC);
    
    // if get_heartbeats fails, ignore
    fake_link->server_set = 0;
    zval_ptr_dtor(&fake_zval);
    efree(errmsg);
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
}

int mongo_util_rs__another_master(zval *response, mongo_link *link TSRMLS_DC) {
  zval **primary;
  char *host;
  mongo_server *server;
  zval *errmsg;
  
  if (zend_hash_find(HASH_P(response), "primary", strlen("primary")+1, (void**)&primary) == FAILURE) {
    // this node can't reach the master, try someone else
    return FAILURE;
  }

  // we're definitely going home
  
  // can't free response until we're done with primary
      
  host = Z_STRVAL_PP(primary);
  if (!(server = mongo_util_rs__find_or_make_server(host, link TSRMLS_CC))) {
    return FAILURE;
  }

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);
        
  if (!server->connected && mongo_util_pool_get(server, errmsg) == FAILURE) {
    zval_ptr_dtor(&errmsg);
    return FAILURE;
  }
  zval_ptr_dtor(&errmsg);

#ifdef DEBUG_CONN
  log2("connected to %s:%d\n", server->host, server->port);
#endif

  // if successful, we're connected to the master
  link->server_set->master = server;
  return SUCCESS;
}

int set_a_slave(mongo_link *link, char **errmsg) {
  mongo_server *possible_slave;
  int skip;
  zval **master = 0, **health = 0;
  
  if (!link->rs || !link->server_set) {
    *(errmsg) = estrdup("Connection is not initialized or not a replica set");
    return FAILURE;
  }

  // pick a secondary S such that S is in [0, num slaves-1)
  skip = (int)rand();
  if (skip < 0) {
    skip *= -1;
  }

  if (link->server_set->slaves) {
    skip %= link->server_set->slaves;
    possible_slave = link->server_set->server;
  }
  // if there are no slaves, don't check for them
  else {
    possible_slave = 0;
  }
  
  // skip to the Sth server
  link->slave = 0;
  while (possible_slave) {      
   
    if (!possible_slave->readable) {
      possible_slave = possible_slave->next;
      continue;
    }
          
    if (skip) {
      skip--;
      possible_slave = possible_slave->next;
      continue;
    }

    // get_heartbeats checks the status of the slave, so we can assume it's okay
    link->slave = possible_slave;
    return RS_SECONDARY;
  }

  // if we've run out of possibilities, use the master
  if (link->server_set->master &&
      zend_hash_find(link->server_set->hosts,
                     link->server_set->master->label,
                     strlen(link->server_set->master->label)+1,
                     (void**)&master) == SUCCESS &&
      Z_TYPE_PP(master) == IS_ARRAY &&
      zend_hash_find(Z_ARRVAL_PP(master), "health", strlen("health")+1,
                     (void**)&health) == SUCCESS &&
      Z_NUMVAL_PP(health, 1)) {

    link->slave = link->server_set->master;
    return RS_PRIMARY;
  }
  
  *errmsg = estrdup("No secondary found");
  return FAILURE;
}

int get_heartbeats(zval *this_ptr, char **errmsg  TSRMLS_DC) {
  zval *db, *name, *data, *result, temp, **members, **ok, **member;
  HashTable *hash;
  HashPosition pointer;
  mongo_link *link;
  mongo_server *ptr;
  
  Z_TYPE(temp) = IS_NULL;

  MAKE_STD_ZVAL(name);
  ZVAL_STRING(name, "admin", 1);
  
  MAKE_STD_ZVAL(db);
  object_init_ex(db, mongo_ce_DB);
  MONGO_METHOD2(MongoDB, __construct, &temp, db, getThis(), name);

  // not sure if this is even possible
  if (EG(exception)) {
    zval_ptr_dtor(&name);
    zval_ptr_dtor(&db);
    
    // but better safe than sorry
    return FAILURE;
  }
  
  MAKE_STD_ZVAL(result);

  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_long(data, "replSetGetStatus", 1);

  MONGO_CMD(result, db);

  zval_ptr_dtor(&name);
  zval_ptr_dtor(&db);
  zval_ptr_dtor(&data);

  if (EG(exception)) {
    return FAILURE;
  }

  // check results for errors
  if (zend_hash_find(Z_ARRVAL_P(result), "members", strlen("members")+1, (void**)&members) == FAILURE ||
      !(zend_hash_find(Z_ARRVAL_P(result), "ok", strlen("ok")+1, (void**)&ok) == SUCCESS &&
        Z_NUMVAL_PP(ok, 1))) {
    zval_ptr_dtor(&result);
    *(errmsg) = estrdup("status msg wasn't valid");
    return FAILURE;
  }


  link = (mongo_link*)zend_object_store_get_object((getThis()) TSRMLS_CC);
  if (!link) {
    zval_ptr_dtor(&result);
    *(errmsg) = estrdup("connection not properly initialized");
    return FAILURE;
  }
  
  link->server_set->slaves = 0;
  // clear readable bit
  ptr = link->server_set->server;
  while (ptr) {
    ptr->readable = 0;
    ptr = ptr->next;
  }

  hash = Z_ARRVAL_PP(members);
  for (zend_hash_internal_pointer_reset_ex(hash, &pointer); 
       zend_hash_get_current_data_ex(hash, (void**) &member, &pointer) == SUCCESS; 
       zend_hash_move_forward_ex(hash, &pointer)) {
    // host is never really used, it's just a temp var
    zval **name = 0, **host = 0, **state = 0, **health = 0, *dest;
    
    if (zend_hash_find(Z_ARRVAL_PP(member), "name", strlen("name")+1,
                       (void**)&name) == FAILURE) {
      // TODO: well, that's weird
      continue;
    }

    // mark servers in state 2 readable
    if (zend_hash_find(Z_ARRVAL_PP(member), "state", strlen("state")+1,
                       (void**)&state) == SUCCESS &&
        zend_hash_find(Z_ARRVAL_PP(member), "health", strlen("health")+1,
                       (void**)&health) == SUCCESS &&
        Z_NUMVAL_PP(state, 2) && Z_NUMVAL_PP(health, 1)) {

      mongo_server *server = link->server_set->server;
      while (server) {
        if (strncmp(server->label, Z_STRVAL_PP(name), strlen(server->label)) == 0) {
          server->readable = 1;
          link->server_set->slaves++;
          break;
        }
        server = server->next;
      }
    }
    
    if (zend_hash_find(link->server_set->hosts, Z_STRVAL_PP(name),
                       Z_STRLEN_PP(name)+1, (void**)&host) == FAILURE) {
#ifdef DEBUG_CONN
      log1("no host named %s", Z_STRVAL_PP(name));
#endif
      continue;
    }
    
    if (mongo_util_hash_to_pzval(&dest, member TSRMLS_CC) == FAILURE) {
      return FAILURE;
    }
    zend_hash_update(link->server_set->hosts, Z_STRVAL_PP(name),
                     Z_STRLEN_PP(name)+1, (void*)&dest, sizeof(zval*), NULL);
  }
  
  zval_ptr_dtor(&result);
  return SUCCESS;
}
