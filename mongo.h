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

#define MSG_HEADER_SIZE 16
#define REPLY_HEADER_SIZE (MSG_HEADER_SIZE+20)
#define INITIAL_BUF_SIZE 4096
#define MAX_RESPONSE_LEN 65536

// duplicate strings
#define DUP 1
#define NO_DUP 0

#define FLAGS 0

#define CREATE_MSG_HEADER(rid, rto, opcode)                     \
  mongo_msg_header header;                                      \
  header.length = 0;                                            \
  header.request_id = rid;                                      \
  header.response_to = rto;                                     \
  header.op = opcode;

#define CREATE_RESPONSE_HEADER(buf, ns, ns_len, rto, opcode)            \
  CREATE_MSG_HEADER(MonGlo(request_id)++, rto, opcode);                 \
  APPEND_HEADER_NS(buf, ns, ns_len);

#define CREATE_HEADER(buf, ns, ns_len, opcode)          \
  CREATE_RESPONSE_HEADER(buf, ns, ns_len, 0, opcode);                    


#define APPEND_HEADER(buf) buf.pos += INT_32;             \
  serialize_int(&buf, header.request_id);                 \
  serialize_int(&buf, header.response_to);                \
  serialize_int(&buf, header.op);                         \
  serialize_int(&buf, 0);                                


#define APPEND_HEADER_NS(buf, ns, ns_len)               \
  APPEND_HEADER(buf);                                   \
  serialize_string(&buf, ns, ns_len);              

#define GET_RESPONSE(link, cursor)              \
  get_reply(link, cursor)

#define GET_RESPONSE_NS(link, cursor, nsp, nsp_len)      \
  if (GET_RESPONSE(link, cursor) == SUCCESS) {           \
    cursor->ns = nsp;                                    \
    cursor->ns_len = nsp_len;                            \
  }


typedef struct {
  int socket;
  int connected;
  int ts;

  char *host;
  int port;

  char *username;
  char *password;

} mongo_link;

typedef struct {
  int length;
  int request_id;
  int response_to;
  int op;
} mongo_msg_header;

typedef struct {
  unsigned char *start;
  unsigned char *pos;
  unsigned char *end;
} buffer;

typedef struct {
  int ref_count;

  // response header
  mongo_msg_header header;
  // connection
  mongo_link link;
  // collection namespace
  char *ns;
  int ns_len;

  // response fields
  int flag;
  long cursor_id;
  int start;

  // number of results used
  int at;
  // number results returned
  int num;
  // total number to return
  int limit;

  // results
  char *buf;
  char *buf_start;
  int buf_size;
  int pos;
} mongo_cursor;

#define BUF_REMAINING (buf->end-buf->pos)

#define CREATE_BUF(buf, size) buffer buf;               \
  buf.start = (unsigned char*)emalloc(size);            \
  buf.pos = buf.start;                                  \
  buf.end = buf.start + size;

#define DEBUG_BUF                                   \
  unsigned char *temp = buf.start;                  \
  while(temp != buf.pos) {                          \
    php_printf("%d\n", *temp++);                    \
  }



PHP_MINIT_FUNCTION(mongo);
PHP_MSHUTDOWN_FUNCTION(mongo);
PHP_RINIT_FUNCTION(mongo);
PHP_MINFO_FUNCTION(mongo);

PHP_FUNCTION(mongo_connect);
PHP_FUNCTION(mongo_close);
PHP_FUNCTION(mongo_query);
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

static int say(mongo_link*, buffer* TSRMLS_DC);
static int check_connection(mongo_link* TSRMLS_DC);
static int mongo_connect(mongo_link*);
static int get_sockaddr(struct sockaddr_in*, char*, int);
static void php_mongo_do_connect(INTERNAL_FUNCTION_PARAMETERS);
static int get_reply(mongo_link*, mongo_cursor*);
static void kill_cursor(mongo_cursor* TSRMLS_DC);

extern zend_module_entry mongo_module_entry;
#define phpext_mongo_ptr &mongo_module_entry

#endif
