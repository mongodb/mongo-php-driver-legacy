// server.c
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

#include "../php_mongo.h"
#include "rs.h"
#include "server.h"

extern int le_pserver;

int mongo_util_server_ping(mongo_server *server, time_t now TSRMLS_DC) {
  server_info* info;
  zval *response = 0, **secondary = 0, **bson = 0;
  struct timeval start, end;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  info->pinged = 1;

  if (info->last_ping + MONGO_PING_INTERVAL > now) {
    return FAILURE;
  }

  gettimeofday(&start, 0);
  response = mongo_util_rs__ismaster(server TSRMLS_CC);
  gettimeofday(&end, 0);

  mongo_util_server__set_ping(info, start, end);

  if (!response) {
    return FAILURE;
  }

  zend_hash_find(HASH_P(response), "secondary", strlen("secondary")+1, (void**)&secondary);
  if (secondary && Z_BVAL_PP(secondary)) {
    info->readable = 1;
  }
  else {
    info->readable = 0;
  }

  zend_hash_find(HASH_P(response), "maxBsonObjectSize", strlen("maxBsonObjectSize")+1, (void**)&bson);
  if (bson) {
    if (Z_TYPE_PP(bson) == IS_LONG) {
      info->max_bson_size = Z_LVAL_PP(bson);
    }
    else if (Z_TYPE_PP(bson) == IS_DOUBLE) {
      info->max_bson_size = (int)Z_DVAL_PP(bson);
    }
    // otherwise, leave as the default
  }

  if (mongo_util_rs__get_ismaster(response TSRMLS_CC)) {
    info->master = 1;
    info->readable = 1;
    zval_ptr_dtor(&response);
    return SUCCESS;
  }

  zval_ptr_dtor(&response);
  return FAILURE;
}

int mongo_util_server_get_ping_time(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  if (!info->pinged) {
    mongo_util_server_ping(server, MONGO_SERVER_PING TSRMLS_CC);
  }

  return info->ping;
}

int mongo_util_server__set_ping(server_info *info, struct timeval start, struct timeval end) {
  info->last_ping = start.tv_sec;

  // in microsecs
  info->ping = (end.tv_sec - start.tv_sec)*1000000+(end.tv_usec - start.tv_usec);

  // clocks might return weird stuff
  if (info->ping < 0) {
    info->ping = 0;
  }

  return info->ping;
}

int mongo_util_server_get_bson_size(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return MONGO_SERVER_BSON;
  }

  if (!info->pinged) {
    mongo_util_server_ping(server, MONGO_SERVER_PING TSRMLS_CC);
  }

  return info->max_bson_size;
}

int mongo_util_server_set_readable(mongo_server *server, zend_bool readable TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return FAILURE;
  }

  info->readable = readable;
  return SUCCESS;
}

int mongo_util_server_get_readable(mongo_server *server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return 0;
  }

  if (!info->pinged) {
    mongo_util_server_ping(server, MONGO_SERVER_PING TSRMLS_CC);
  }

  return info->readable;
}

void mongo_util_server_down(mongo_server* server TSRMLS_DC) {
  server_info* info;

  if ((info = mongo_util_server__get_info(server TSRMLS_CC)) == 0) {
    return;
  }

  info->readable = 0;
  info->master = 0;
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
    zend_rsrc_list_entry nle;
    server_info *info;

    info = (server_info*)malloc(sizeof(server_info));
    if (!info) {
      efree(id);
      return 0;
    }

    memset(info, 0, sizeof(server_info));
    info->ping = MONGO_SERVER_PING;
    info->max_bson_size = MONGO_SERVER_BSON;

    // registering this links it to the dtor (mongo_util_pool_shutdown) so that
    // it can be auto-cleaned-up on shutdown
    nle.ptr = info;
    nle.type = le_pserver;
    nle.refcount = 1;
    zend_hash_add(&EG(persistent_list), id, strlen(id)+1, &nle, sizeof(zend_rsrc_list_entry), NULL);

    efree(id);
    return info;
  }

  efree(id);
  return le->ptr;
}

#ifdef WIN32
void gettimeofday(struct timeval *t, void* tz) {
  SYSTEMTIME ft;

  if (t != 0) {
    GetSystemTime(&ft);

    t->tv_sec = ft.wSecond;
    t->tv_usec = fs.wMilliseconds*1000;
  }
}
#endif

void mongo_util_server_shutdown(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  if (!rsrc) {
    return;
  }

  rsrc->ptr = 0;
}
