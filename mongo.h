/**
 *  Copyright 2009 10gen, Inc.
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

#ifndef PHP_MONGO_H
#define PHP_MONGO_H 1

#define PHP_MONGO_VERSION "1.0"
#define PHP_MONGO_EXTNAME "mongo"

#define PHP_CONNECTION_RES_NAME "mongo connection"
#define PHP_AUTH_CONNECTION_RES_NAME "mongo authenticated connection"
#define PHP_DB_CURSOR_RES_NAME "mongo cursor"
#define PHP_GRIDFS_RES_NAME "gridfs tools"
#define PHP_GRIDFILE_RES_NAME "gridfs file"
#define PHP_GRIDFS_CHUNK_RES_NAME "gridfs file chunk"

#define OP_REPLY  1
#define OP_MSG 1000
#define OP_UPDATE 2001
#define OP_INSERT 2002
#define OP_GET_BY_OID 2003
#define OP_QUERY 2004
#define OP_GET_MORE 2005 
#define OP_DELETE 2006
#define OP_KILL_CURSORS 2007 

#define CREATE_INSERT_HEADER                                    \
  mongo_msg_header header;                                      \
  header.length = 0;                                            \
  header.request_id = MonGlo(request_id)++;                     \
  header.response_to = 0;                                       \
  header.op = OP_INSERT;


#define APPEND_HEADER(buf, pos)                         \
  pos += INT_32;                                        \
  memcpy(buf+pos, &(header.request_id), INT_32);        \
  pos += INT_32;                                        \
  memcpy(buf+pos, &(header.response_to), INT_32);       \
  pos += INT_32;                                        \
  memcpy(buf+pos, &(header.op), INT_32);                \
  pos += INT_32;                                        \
  memset(buf+pos, 0, INT_32);                           \
  pos += INT_32;                                        


#define APPEND_HEADER_NS(buf, pos, ns, ns_len)          \
  APPEND_HEADER(buf, pos)                               \
  memcpy(buf+pos, ns, ns_len);                          \
  pos += ns_len + BYTE_8;



typedef struct {
  int socket;
} mongo_link;

typedef struct {
  int length;
  int request_id;
  int response_to;
  int op;
} mongo_msg_header;

PHP_MINIT_FUNCTION(mongo);
PHP_MSHUTDOWN_FUNCTION(mongo);
PHP_RINIT_FUNCTION(mongo);
PHP_MINFO_FUNCTION(mongo);

PHP_FUNCTION(mongo_connect);
PHP_FUNCTION(mongo_close);
PHP_FUNCTION(mongo_query);
PHP_FUNCTION(mongo_find_one);
PHP_FUNCTION(mongo_remove);
PHP_FUNCTION(mongo_insert);
PHP_FUNCTION(mongo_batch_insert);
PHP_FUNCTION(mongo_update);

PHP_FUNCTION(mongo_has_next);
PHP_FUNCTION(mongo_next);

ZEND_BEGIN_MODULE_GLOBALS(mongo)
long num_links,num_persistent;
long max_links,max_persistent;
long allow_persistent; 
bool auto_reconnect; 
char *default_host; 
long default_port;
int request_id; 
ZEND_END_MODULE_GLOBALS(mongo) 

#ifdef ZTS
#include <TSRM.h>
# define MonGlo(v) TSRMG(mongo_globals_id, zend_mongo_globals *, v)
#else
# define MonGlo(v) (mongo_globals.v)
#endif 

static void php_mongo_do_connect(INTERNAL_FUNCTION_PARAMETERS);

extern zend_module_entry mongo_module_entry;
#define phpext_mongo_ptr &mongo_module_entry

#endif
