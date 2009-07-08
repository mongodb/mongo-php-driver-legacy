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

// resource names
#define PHP_CONNECTION_RES_NAME "mongo connection"
#define PHP_AUTH_CONNECTION_RES_NAME "mongo authenticated connection"

// db ops
#define OP_REPLY 1
#define OP_MSG 1000
#define OP_UPDATE 2001
#define OP_INSERT 2002
#define OP_GET_BY_OID 2003
#define OP_QUERY 2004
#define OP_GET_MORE 2005 
#define OP_DELETE 2006
#define OP_KILL_CURSORS 2007 

// cursor flags
#define CURSOR_NOT_FOUND 1
#define CURSOR_ERR 2

#define MSG_HEADER_SIZE 16
#define REPLY_HEADER_SIZE (MSG_HEADER_SIZE+20)
#define INITIAL_BUF_SIZE 4096
// should only be 4Mb
#define MAX_RESPONSE_LEN 5242880
#define DEFAULT_CHUNK_SIZE (256*1024)

// if _id field should be added
#define PREP 1
#define NO_PREP 0

#define LAZY 1
#define NOT_LAZY 0

#define NOISY 0

// duplicate strings
#define DUP 1
#define NO_DUP 0

#define FLAGS 0

#if ZEND_MODULE_API_NO >= 20090115
# define PUSH_PARAM(arg) zend_vm_stack_push(arg TSRMLS_CC)
# define POP_PARAM() zend_vm_stack_pop(TSRMLS_C)
# define PUSH_EO_PARAM()
# define POP_EO_PARAM()
#else
# define PUSH_PARAM(arg) zend_ptr_stack_push(&EG(argument_stack), arg)
# define POP_PARAM() zend_ptr_stack_pop(&EG(argument_stack))
# define PUSH_EO_PARAM() zend_ptr_stack_push(&EG(argument_stack), NULL)
# define POP_EO_PARAM() zend_ptr_stack_pop(&EG(argument_stack))
#endif

#if ZEND_MODULE_API_NO >= 20060613
// normal, nice method
#define MONGO_METHOD(classname, name) zim_##classname##_##name
#else
// gah!  wtf, php 5.1?
#define MONGO_METHOD(classname, name) zif_##classname##_##name
#endif /* ZEND_MODULE_API_NO >= 20060613 */

typedef struct {
  int ts;
  int paired;
  int persist;
  int master;

  union {
    struct {
      char *host;
      int port;

      int socket;
      int connected;
    } single;
    struct {
      char *left;
      int lport;
      int lsocket;
      int lconnected;

      char *right;
      int rport;
      int rsocket;
      int rconnected;
    } paired;
  } server;

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

#define CREATE_MSG_HEADER(rid, rto, opcode)                     \
  header.length = 0;                                            \
  header.request_id = rid;                                      \
  header.response_to = rto;                                     \
  header.op = opcode;

#define CREATE_RESPONSE_HEADER(buf, ns, ns_len, rto, opcode)            \
  CREATE_MSG_HEADER(MonGlo(request_id)++, rto, opcode);                 \
  APPEND_HEADER_NS(buf, ns, ns_len, 0);

#define CREATE_HEADER_WITH_OPTS(buf, ns, opcode, opts)  \
  CREATE_MSG_HEADER(MonGlo(request_id)++, 0, opcode);   \
  APPEND_HEADER_NS(buf, ns, strlen(ns), opts);

#define CREATE_HEADER(buf, ns, ns_len, opcode)          \
  CREATE_RESPONSE_HEADER(buf, ns, ns_len, 0, opcode);                    


#define APPEND_HEADER(buf, opts) buf.pos += INT_32;       \
  serialize_int(&buf, header.request_id);                 \
  serialize_int(&buf, header.response_to);                \
  serialize_int(&buf, header.op);                         \
  serialize_int(&buf, opts);                                


#define APPEND_HEADER_NS(buf, ns, ns_len, opts)         \
  APPEND_HEADER(buf, opts);                             \
  serialize_string(&buf, ns, ns_len);              


#define MONGO_CHECK_INITIALIZED(member, class_name)                     \
  if (!(member)) {                                                      \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "The " #class_name " object has not been correctly initialized by its constructor"); \
    RETURN_FALSE;                                                       \
  }

#define MONGO_CHECK_INITIALIZED_STRING(member, class_name)              \
  if (!(member)) {                                                      \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "The " #class_name " object has not been correctly initialized by its constructor"); \
    RETURN_STRING("", 1);                                               \
  }


typedef struct {
  zend_object std;

  // connection
  mongo_link *link;
  zval *resource;

  // collection namespace
  char *ns;

  // fields to send
  zval *query;
  zval *fields;
  int limit;
  int skip;
  int opts;

  // response header
  mongo_msg_header header;
  // response fields
  int flag;
  long long cursor_id;
  int start;
  // number of results used
  int at;
  // number results returned
  int num;
  // results
  buffer buf;

  zend_bool started_iterating;

  zval *current;

} mongo_cursor;


typedef struct {
  zend_object std;
  char *id;
} mongo_id;


typedef struct {
  zend_object std;
  zval *link;
  zval *name;
} mongo_db;

typedef struct {
  zend_object std;

  // parent database
  zval *parent;

  // db obj
  mongo_db *db;

  // names
  zval *name;
  zval *ns;
} mongo_collection;


#define BUF_REMAINING (buf->end-buf->pos)

#define CREATE_BUF(buf, size) buffer buf;               \
  buf.start = (unsigned char*)emalloc(size);            \
  buf.pos = buf.start;                                  \
  buf.end = buf.start + size;

#define DEBUG_BUF(buf)                              \
  unsigned char *temp = buf.start;                  \
  while(temp != buf.pos) {                          \
    php_printf("%d\n", *temp++);                    \
  }

PHP_MINIT_FUNCTION(mongo);
PHP_MSHUTDOWN_FUNCTION(mongo);
PHP_RINIT_FUNCTION(mongo);
PHP_MINFO_FUNCTION(mongo);


/*
 * Mongo class
 */
PHP_METHOD(Mongo, __construct);
PHP_METHOD(Mongo, connect);
PHP_METHOD(Mongo, pairConnect);
PHP_METHOD(Mongo, persistConnect);
PHP_METHOD(Mongo, pairPersistConnect);
PHP_METHOD(Mongo, connectUtil);
PHP_METHOD(Mongo, __toString);
PHP_METHOD(Mongo, selectDB);
PHP_METHOD(Mongo, selectCollection);
PHP_METHOD(Mongo, dropDB);
PHP_METHOD(Mongo, repairDB);
PHP_METHOD(Mongo, lastError);
PHP_METHOD(Mongo, prevError);
PHP_METHOD(Mongo, resetError);
PHP_METHOD(Mongo, forceError);
PHP_METHOD(Mongo, close);
PHP_METHOD(Mongo, __destruct);


/*
 * Internal functions
 */
void mongo_do_up_connect_caller(INTERNAL_FUNCTION_PARAMETERS);
void mongo_do_connect_caller(INTERNAL_FUNCTION_PARAMETERS, zval *username, zval *password);
int mongo_say(mongo_link*, buffer* TSRMLS_DC);
int mongo_hear(mongo_link*, void*, int TSRMLS_DC);
int get_reply(mongo_cursor* TSRMLS_DC);

void mongo_init_Mongo(TSRMLS_D);
void mongo_init_MongoDB(TSRMLS_D);
void mongo_init_MongoCollection(TSRMLS_D);
void mongo_init_MongoCursor(TSRMLS_D);

void mongo_init_MongoGridFS(TSRMLS_D);
void mongo_init_MongoGridFSFile(TSRMLS_D);
void mongo_init_MongoGridFSCursor(TSRMLS_D);

void mongo_init_MongoId(TSRMLS_D);
void mongo_init_MongoCode(TSRMLS_D);
void mongo_init_MongoRegex(TSRMLS_D);
void mongo_init_MongoDate(TSRMLS_D);
void mongo_init_MongoBinData(TSRMLS_D);
void mongo_init_MongoDBRef(TSRMLS_D);
void mongo_init_MongoEmptyObj(TSRMLS_D);

ZEND_BEGIN_MODULE_GLOBALS(mongo)
long num_links,num_persistent;
long max_links,max_persistent;
long allow_persistent; 
int auto_reconnect; 
char *default_host; 
long default_port;
int request_id; 
int chunk_size;
ZEND_END_MODULE_GLOBALS(mongo) 

#ifdef ZTS
#include <TSRM.h>
# define MonGlo(v) TSRMG(mongo_globals_id, zend_mongo_globals *, v)
#else
# define MonGlo(v) (mongo_globals.v)
#endif 

mongo_cursor* mongo_do_query(mongo_link*, char*, int, int, zval*, zval* TSRMLS_DC);
int mongo_do_has_next(mongo_cursor* TSRMLS_DC);
zval* mongo_do_next(mongo_cursor* TSRMLS_DC);

extern zend_module_entry mongo_module_entry;
#define phpext_mongo_ptr &mongo_module_entry

#endif
