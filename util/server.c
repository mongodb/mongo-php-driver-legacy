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
#include "../db.h"
#include "rs.h"
#include "server.h"
#include "log.h"

extern int le_pserver;
extern zend_class_entry *mongo_ce_Id;

static server_info* create_info();
static void make_other_le(const char *id, server_info *info TSRMLS_DC);
static server_info* wrap_other_guts(server_info *source);

int mongo_util_server_ping(mongo_server *server, time_t now TSRMLS_DC) {
  server_info* info;
  zval *response = 0, **secondary = 0, **bson = 0, **self = 0;
  struct timeval start, end;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  if (info->guts->last_ping + MONGO_PING_INTERVAL > now) {
    return info->guts->master ? SUCCESS : FAILURE;
  }

  gettimeofday(&start, 0);
  response = mongo_util_rs__ismaster(server TSRMLS_CC);
  gettimeofday(&end, 0);

  mongo_util_server__set_ping(info, start, end);

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
  else {
    if (info->guts->readable) {
      mongo_log(MONGO_LOG_SERVER, MONGO_LOG_INFO TSRMLS_CC, "server: %s is now not readable", server->label);
    }

    info->guts->readable = 0;
    info->guts->master = 0;
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

  if (mongo_util_rs__get_ismaster(response TSRMLS_CC)) {
    if (!info->guts->master) {
      mongo_log(MONGO_LOG_SERVER, MONGO_LOG_INFO TSRMLS_CC, "server: %s is now primary", server->label);
    }

    info->guts->master = 1;
    info->guts->readable = 1;
    zval_ptr_dtor(&response);
    return SUCCESS;
  }

  zval_ptr_dtor(&response);
  return FAILURE;
}

void mongo_util_server__prime(server_info *info, mongo_server *server TSRMLS_DC) {
  if (info->guts->ping) {
    return;
  }

  mongo_util_server_ping(server, MONGO_SERVER_PING TSRMLS_CC);
}

int mongo_util_server_get_ping_time(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  mongo_util_server__prime(info, server TSRMLS_CC);

  return info->guts->ping;
}

int mongo_util_server_get_bucket(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  mongo_util_server__prime(info, server TSRMLS_CC);

  return info->guts->bucket;
}

int mongo_util_server__set_ping(server_info *info, struct timeval start, struct timeval end) {
  info->guts->last_ping = start.tv_sec;

  // in microsecs
  info->guts->ping = (end.tv_sec - start.tv_sec)*1000000+(end.tv_usec - start.tv_usec);

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

  info->guts->readable = 0;
  info->guts->master = 0;
}

server_info* mongo_util_server__get_info(mongo_server *server TSRMLS_DC) {
  zend_rsrc_list_entry *le = 0;
  char *id;

  id = (char*)emalloc(strlen(server->label)+strlen(MONGO_SERVER_INFO)+2);
  memcpy(id, MONGO_SERVER_INFO, strlen(MONGO_SERVER_INFO));
  memcpy(id+strlen(MONGO_SERVER_INFO), ":", 1);
  memcpy(id+strlen(MONGO_SERVER_INFO)+1, server->label, strlen(server->label));
  id[strlen(MONGO_SERVER_INFO)+strlen(server->label)+1] = 0;

  if (zend_hash_find(&EG(persistent_list), id, strlen(id)+1, (void**)&le) == FAILURE) {
    zend_rsrc_list_entry nle, *other_le = 0;
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
