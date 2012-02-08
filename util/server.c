// server.c
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
#include "../db.h"

#include "rs.h"
#include "server.h"
#include "log.h"
#include "pool.h"

extern int le_pserver;
extern zend_class_entry *mongo_ce_Id;

static server_info* create_info();
static void make_other_le(const char *id, server_info *info TSRMLS_DC);
static server_info* wrap_other_guts(server_info *source);
static char* get_server_id(char *host);
static void mongo_util_server__down(server_info *server);
// we only want to call this every INTERVAL seconds
static int mongo_util_server_reconnect(mongo_server *server TSRMLS_DC);

mongo_server* mongo_util_server_copy(const mongo_server *source, mongo_server *dest, int persist TSRMLS_DC) {
  // we assume if def was persistent it will still be persistent and visa versa

  if (dest) {
    php_mongo_server_free(dest, persist TSRMLS_CC);
  }

  dest = (mongo_server*)pemalloc(sizeof(mongo_server), persist);
  memset(dest, 0, sizeof(mongo_server));

  dest->host = pestrdup(source->host, persist);
  dest->port = source->port;
  dest->label = pestrdup(source->label, persist);

  if (source->username && source->password && source->db) {
    dest->username = pestrdup(source->username, persist);
    dest->password = pestrdup(source->password, persist);
    dest->db = pestrdup(source->db, persist);
  }

  mongo_util_pool_get(dest, 0 TSRMLS_CC);

  return dest;
}

int mongo_util_server_cmp(char *host1, char *host2 TSRMLS_DC) {
  char *id1 = 0, *id2 = 0;
  int result = 0;
  zend_rsrc_list_entry *le1 = 0, *le2 = 0;

  id1 = get_server_id(host1);
  id2 = get_server_id(host2);

  if (zend_hash_find(&EG(persistent_list), id1, strlen(id1)+1, (void**)&le1) == SUCCESS &&
      zend_hash_find(&EG(persistent_list), id2, strlen(id2)+1, (void**)&le2) == SUCCESS &&
      ((server_info*)le1->ptr)->guts == ((server_info*)le2->ptr)->guts) {
    mongo_log(MONGO_LOG_SERVER, MONGO_LOG_INFO TSRMLS_CC, "server: detected that %s is the same server as %s",
              host1, host2);
    // result is initialized to 0
  }
  else {
    result = strcmp(id1, id2);
  }

  efree(id1);
  efree(id2);

  return result;
}

static int mongo_util_server_reconnect(mongo_server *server TSRMLS_DC) {
  // if the server is down, try to reconnect
  if (!server->connected &&
      mongo_util_pool_refresh(server, MONGO_RS_TIMEOUT TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  return SUCCESS;
}

int mongo_util_server_ping(mongo_server *server, time_t now TSRMLS_DC) {
  server_info* info;
  zval *response = 0, **ok = 0;
  struct timeval start, end;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  // call ismaster every ISMASTER_INTERVAL seconds
  if (info->guts->last_ismaster + MONGO_ISMASTER_INTERVAL <= now) {
    if (mongo_util_server_reconnect(server TSRMLS_CC) == FAILURE) {
      return FAILURE;
    }

	// we don't return here, because ismaster doesn't actually do the
	// ping
	mongo_util_server_ismaster(info, server, now TSRMLS_CC);
  }

  if (info->guts->last_ping + MONGO_PING_INTERVAL > now) {
    return info->guts->readable ? SUCCESS : FAILURE;
  }

  if (mongo_util_server_reconnect(server TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  gettimeofday(&start, 0);
  response = mongo_util_rs__cmd("ping", server TSRMLS_CC);
  gettimeofday(&end, 0);

  mongo_util_server__set_ping(info, start, end);

  if (!response) {
    mongo_util_server__down(info);
    return FAILURE;
  }

  // TODO: clear exception?

  zend_hash_find(HASH_P(response), "ok", strlen("ok")+1, (void**)&ok);
  if (Z_NUMVAL_PP(ok, 1) &&
      info->guts->last_ismaster + MONGO_ISMASTER_INTERVAL <= now) {
    mongo_util_server_ismaster(info, server, now TSRMLS_CC);
  }
  zval_ptr_dtor(&response);

  return info->guts->readable ? SUCCESS : FAILURE;
}

int mongo_util_server_ismaster(server_info *info, mongo_server *server, time_t now TSRMLS_DC) {
  zval *response = 0, **secondary = 0, **bson = 0, **self = 0;

  response = mongo_util_rs__cmd("ismaster", server TSRMLS_CC);

  // update last_ismaster
  info->guts->last_ismaster = now;

  if (!response) {
    return FAILURE;
  }

  zend_hash_find(HASH_P(response), "me", strlen("me")+1, (void**)&self);
  if (!info->guts->pinged && self &&
      strncmp(Z_STRVAL_PP(self), server->label, Z_STRLEN_PP(self)) != 0) {

    // this server thinks its name is different than what we have recorded
    mongo_log(MONGO_LOG_SERVER, MONGO_LOG_INFO TSRMLS_CC, "server: found another name for %s: %s",
              server->label, Z_STRVAL_PP(self));

    // make a new info entry for this name, pointing to our info
    make_other_le(Z_STRVAL_PP(self), info TSRMLS_CC);
  }

  // now we have pinged it at least once
  info->guts->pinged = 1;

  zend_hash_find(HASH_P(response), "secondary", strlen("secondary")+1, (void**)&secondary);
  if (secondary && Z_BVAL_PP(secondary)) {
    if (!info->guts->readable) {
      mongo_log(MONGO_LOG_SERVER, MONGO_LOG_INFO TSRMLS_CC, "server: %s is now a secondary", server->label);
    }

    info->guts->readable = 1;
    info->guts->master = 0;
  }
  else if (mongo_util_rs__get_ismaster(response TSRMLS_CC)) {
    if (!info->guts->master) {
      mongo_log(MONGO_LOG_SERVER, MONGO_LOG_INFO TSRMLS_CC, "server: %s is now primary", server->label);
    }

    info->guts->master = 1;
    info->guts->readable = 1;
  }
  else {
    if (info->guts->readable) {
      mongo_log(MONGO_LOG_SERVER, MONGO_LOG_INFO TSRMLS_CC, "server: %s is now not readable", server->label);
    }

    mongo_util_server__down(info);
  }

  zend_hash_find(HASH_P(response), "maxBsonObjectSize", strlen("maxBsonObjectSize")+1, (void**)&bson);
  if (bson) {
    if (Z_TYPE_PP(bson) == IS_LONG) {
      info->guts->max_bson_size = Z_LVAL_PP(bson);
    }
    else if (Z_TYPE_PP(bson) == IS_DOUBLE) {
      info->guts->max_bson_size = (int)Z_DVAL_PP(bson);
    }
    // otherwise, leave as the default
    else {
      mongo_log(MONGO_LOG_SERVER, MONGO_LOG_WARNING TSRMLS_CC,
                "server: could not find max bson size on %s, consider upgrading your server", server->label);
    }
  }

  zval_ptr_dtor(&response);
  return SUCCESS;
}

void mongo_util_server__prime(server_info *info, mongo_server *server TSRMLS_DC) {
  if (info->guts->pinged) {
    return;
  }

  mongo_util_server_ping(server, MONGO_SERVER_PING TSRMLS_CC);
}

int mongo_util_server_get_bucket(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  mongo_util_server__prime(info, server TSRMLS_CC);

  return info->guts->bucket;
}

int mongo_util_server_get_state(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return 0;
  }

  mongo_util_server__prime(info, server TSRMLS_CC);

  if (info->guts->master) {
    return 1;
  }
  else if (info->guts->readable) {
    return 2;
  }
  else {
    return 0;
  }
}

int mongo_util_server__set_ping(server_info *info, struct timeval start, struct timeval end) {
  info->guts->last_ping = start.tv_sec;

  // in microsecs
  info->guts->ping = (end.tv_sec - start.tv_sec)*1000+(end.tv_usec - start.tv_usec)/1000;

  // clocks might return weird stuff
  if (info->guts->ping < 0) {
    info->guts->ping = 0;
  }

  // buckets for 0, 16, 256, etc.
  if (!info->guts->ping) {
    info->guts->bucket = 0;
  }
  else {
    int temp_ping = info->guts->ping;
    info->guts->bucket = 0;

    while (temp_ping) {
      temp_ping /= 16;
      info->guts->bucket++;
    }
  }

  return info->guts->ping;
}

int mongo_util_server_get_bson_size(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return MONGO_SERVER_BSON;
  }

  mongo_util_server__prime(info, server TSRMLS_CC);

  return info->guts->max_bson_size;
}

int mongo_util_server_set_readable(mongo_server *server, zend_bool readable TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  info->guts->readable = readable;
  return SUCCESS;
}

int mongo_util_server_get_readable(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return 0;
  }

  mongo_util_server__prime(info, server TSRMLS_CC);

  return info->guts->readable;
}

void mongo_util_server_down(mongo_server* server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return;
  }

  mongo_util_server__down(info);
}

void mongo_util_server__down(server_info *info) {
  info->guts->readable = 0;
  info->guts->master = 0;
}

static char* get_server_id(char *host) {
  char *id;

  id = (char*)emalloc(strlen(host)+strlen(MONGO_SERVER_INFO)+2);
  mongo_buf_init(id);
  mongo_buf_append(id, MONGO_SERVER_INFO);
  mongo_buf_append(id, ":");
  mongo_buf_append(id, host);

  return id;
}

server_info* mongo_util_server__get_info(mongo_server *server TSRMLS_DC) {
  zend_rsrc_list_entry *le = 0;
  char *id = get_server_id(server->label);

  if (zend_hash_find(&EG(persistent_list), id, strlen(id)+1, (void**)&le) == FAILURE) {
    zend_rsrc_list_entry nle;
    server_info *info = 0;

    // clear to start so that we can check it was set later
    nle.ptr = 0;

    info = create_info();
    if (info != 0) {
      info->owner = 1;

      nle.ptr = info;
      nle.refcount = 1;
      nle.type = le_pserver;
    }

    if (nle.ptr) {
      zend_hash_add(&EG(persistent_list), id, strlen(id)+1, &nle, sizeof(zend_rsrc_list_entry), NULL);
    }

    efree(id);
    return info;
  }

  efree(id);
  return le->ptr;
}

static server_info* wrap_other_guts(server_info *source) {
  server_info *info;

  info = (server_info*)pemalloc(sizeof(server_info), 1);
  info->owner = 0;
  info->guts = source->guts;

  return info;
}

static server_info* create_info() {
  server_info *info;
  server_guts *guts;

  info = (server_info*)pemalloc(sizeof(server_info), 1);
  guts = (server_guts*)pemalloc(sizeof(server_guts), 1);

  memset(guts, 0, sizeof(server_guts));
  guts->ping = MONGO_SERVER_PING;
  guts->max_bson_size = MONGO_SERVER_BSON;

  memset(info, 0, sizeof(server_info));
  info->guts = guts;

  return info;
}

static void make_other_le(const char *id, server_info *info TSRMLS_DC) {
  zend_rsrc_list_entry *le = 0, nle;

  if (zend_hash_find(&EG(persistent_list), id, strlen(id)+1, (void**)&le) == SUCCESS) {
    return;
  }

  nle.ptr = wrap_other_guts(info);
  nle.type = le_pserver;
  nle.refcount = 1;

  zend_hash_add(&EG(persistent_list), id, strlen(id)+1, &nle, sizeof(zend_rsrc_list_entry), NULL);
}

#ifdef WIN32
void gettimeofday(struct timeval *t, void* tz) {
  SYSTEMTIME ft;

  if (t != 0) {
    GetSystemTime(&ft);

    t->tv_sec = ft.wSecond;
    t->tv_usec = ft.wMilliseconds*1000;
  }
}
#endif

void mongo_util_server_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  server_info *info;

  if (!rsrc || !rsrc->ptr) {
    return;
  }

  info = (server_info*)rsrc->ptr;
  if (info->owner) {
    pefree(info->guts, 1);
    info->guts = 0;
  }

  pefree(info, 1);
  rsrc->ptr = 0;
}

PHP_METHOD(Mongo, serverInfo) {
  HashPosition pointer;
  zend_rsrc_list_entry *le;

  array_init(return_value);

  for (zend_hash_internal_pointer_reset_ex(&EG(persistent_list), &pointer);
       zend_hash_get_current_data_ex(&EG(persistent_list), (void**) &le, &pointer) == SUCCESS;
       zend_hash_move_forward_ex(&EG(persistent_list), &pointer)) {
    zval *m;
    char *key;
    unsigned int key_len;
    unsigned long index;
    server_info *info;

    if (!le || le->type != le_pserver) {
      continue;
    }

    info = (server_info*)le->ptr;

    MAKE_STD_ZVAL(m);
    array_init(m);

    add_assoc_bool(m, "owner", info->owner);
    add_assoc_long(m, "last ping", info->guts->last_ping);
    add_assoc_long(m, "ping (ms)", info->guts->ping);
    add_assoc_long(m, "master", info->guts->master);
    add_assoc_long(m, "readable", info->guts->readable);
    add_assoc_long(m, "max BSON size", info->guts->max_bson_size);

    if (zend_hash_get_current_key_ex(&EG(persistent_list), &key, &key_len, &index, 0, &pointer) == HASH_KEY_IS_STRING) {
      add_assoc_zval(return_value, key, m);
    }
    else {
      add_index_zval(return_value, index, m);
    }
  }

  // return_value is returned
}
