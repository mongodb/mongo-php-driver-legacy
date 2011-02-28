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

extern zend_class_entry *mongo_ce_Mongo,
  *mongo_ce_Cursor;
ZEND_EXTERN_MODULE_GLOBALS(mongo);

static mongo_server* find_or_make_server(char *host, mongo_link *link TSRMLS_DC);
static zval* create_fake_cursor(mongo_link *link TSRMLS_DC);

static mongo_server* find_or_make_server(char *host, mongo_link *link TSRMLS_DC) {
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


static zval* create_fake_cursor(mongo_link *link TSRMLS_DC) {
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

mongo_server* mongo_util_rs_get_master(mongo_link *link TSRMLS_DC) {
  zval *cursor_zval;
  mongo_cursor *cursor;
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

  // redetermine master

  // create a cursor
  cursor_zval = create_fake_cursor(link TSRMLS_CC);
  cursor = (mongo_cursor*)zend_object_store_get_object(cursor_zval TSRMLS_CC);

  current = link->server_set->server;
  while (current) {
    zval temp_ret, *response, **hosts, **ans, *errmsg;
    int ismaster = 0, exception = 0, now = 0;
    mongo_link temp;
    mongo_server *temp_next = 0;
    mongo_server_set temp_server_set;

    MAKE_STD_ZVAL(errmsg);
    ZVAL_NULL(errmsg);
    
    // make a fake link
    temp.server_set = &temp_server_set;
    temp.server_set->num = 1;
    temp.server_set->server = current;
    temp.server_set->master = current;
    temp.rs = 0;

    // skip anything we're not connected to
    if (!current->connected && FAILURE == php_mongo_connect_nonb(current, link->timeout, errmsg)) {
#ifdef DEBUG_CONN
      log2("[c:php_mongo_get_master] not connected to %s:%d\n", current->host, current->port);
#endif
      current = current->next;
      zval_ptr_dtor(&errmsg);
      continue;
    }
    zval_ptr_dtor(&errmsg);

    temp_next = current->next;
    current->next = 0;
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

    if (exception || IS_SCALAR_P(response)) {
      zval_ptr_dtor(&response);
      current = current->next;
      continue;
    }

    if (zend_hash_find(HASH_P(response), "ismaster", 9, (void**)&ans) == SUCCESS) {
      // in 1.6.*, this was a float
      if (Z_TYPE_PP(ans) == IS_DOUBLE) {
        ismaster = (Z_DVAL_PP(ans) == 1.0);
      }
      // in 1.7, it became a boolean
      else {
        ismaster = Z_BVAL_PP(ans);
      }
    }

    // check if this is a replica set
    now = time(0);
    if (link->rs && link->server_set->server_ts + 5 < now &&
        zend_hash_find(HASH_P(response), "hosts", strlen("hosts")+1, (void**)&hosts) == SUCCESS) {
      zval **data;
      HashTable *hash;
      HashPosition pointer;
      mongo_server *cur;

      link->server_set->server_ts = now;
      
      // we are going clear the hosts list and repopulate
      cur = link->server_set->server;
      while(cur) {
        mongo_server *prev = cur;
        cur = cur->next;
        php_mongo_server_free(prev TSRMLS_CC);
      }
      link->server_set->server = 0;
      link->server_set->num = 0;
      
#ifdef DEBUG_CONN
      log0("parsing replica set\n");
#endif
        
      // repopulate
      hash = Z_ARRVAL_PP(hosts);
      for (zend_hash_internal_pointer_reset_ex(hash, &pointer); 
           zend_hash_get_current_data_ex(hash, (void**) &data, &pointer) == SUCCESS; 
           zend_hash_move_forward_ex(hash, &pointer)) {
        
        char *host = Z_STRVAL_PP(data);
        
        // this could fail if host is invalid, but it's okay if it does
        find_or_make_server(host, link TSRMLS_CC);
      }

      if (zend_hash_find(HASH_P(response), "passives", strlen("passives")+1, (void**)&hosts) == SUCCESS) {
        hash = Z_ARRVAL_PP(hosts);
        for (zend_hash_internal_pointer_reset_ex(hash, &pointer); 
             zend_hash_get_current_data_ex(hash, (void**) &data, &pointer) == SUCCESS; 
             zend_hash_move_forward_ex(hash, &pointer)) {
        
          char *host = Z_STRVAL_PP(data);
        
          // this could fail if host is invalid, but it's okay if it does
          find_or_make_server(host, link TSRMLS_CC);
        }        
      }
    
      // now that we've replaced the host list, start over
      zval_ptr_dtor(&response);
      current = link->server_set->server;
      continue;
    }
    
    if (!ismaster) {
      zval **primary;
      char *host;
      mongo_server *server;
      zval *errmsg;

      if (zend_hash_find(HASH_P(response), "primary", strlen("primary")+1, (void**)&primary) == FAILURE) {
        // this node can't reach the master, try someone else
        zval_ptr_dtor(&response);
        current = current->next;
        continue;
      }

      // we're definitely going home
      cursor->link = 0;
      zval_ptr_dtor(&cursor_zval);
      // can't free response until we're done with primary
      
      host = Z_STRVAL_PP(primary);
      if (!(server = find_or_make_server(host, link TSRMLS_CC))) {
        zval_ptr_dtor(&response);
        return 0;
      }

      zval_ptr_dtor(&response);
        
      MAKE_STD_ZVAL(errmsg);
      ZVAL_NULL(errmsg);
        
      // TODO: auth, but it won't work in 1.6 anyway
      if (!server->connected && php_mongo_connect_nonb(server, link->timeout, errmsg) == FAILURE) {
        zval_ptr_dtor(&errmsg);
        return 0;
      }
      zval_ptr_dtor(&errmsg);

#ifdef DEBUG_CONN
      log2("connected to %s:%d\n", server->host, server->port);
#endif

      // if successful, we're connected to the master
      link->server_set->master = server;
      return link->server_set->master;
    }

    // reset response
    zval_ptr_dtor(&response);

    if (ismaster) {
      cursor->link = 0;
      zval_ptr_dtor(&cursor_zval);

      link->server_set->master = current;
      return current;
    }
    current = current->next;
  }

  cursor->link = 0;
  zval_ptr_dtor(&cursor_zval);
  return 0;
}


