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

#define PHP_MONGO_VERSION "1.0.4"
#define PHP_MONGO_EXTNAME "mongo"

// resource names
#define PHP_CONNECTION_RES_NAME "mongo connection"
#define PHP_CURSOR_LIST_RES_NAME "cursor list"

#ifdef WIN32
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#endif

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
#define MAX_OBJECT_LEN 4194304
#define DEFAULT_CHUNK_SIZE (256*1024)
#define INVALID_STRING_LEN(len) (len < 0 || len > MAX_RESPONSE_LEN)

// if _id field should be added
#define PREP 1
#define NO_PREP 0

#define NOISY 0

// duplicate strings
#define DUP 1
#define NO_DUP 0

#define FLAGS 0

#define LAST_ERROR 0
#define PREV_ERROR 1
#define RESET_ERROR 2
#define FORCE_ERROR 3

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
#define MONGO_METHOD_BASE(classname, name) zim_##classname##_##name
#else
// gah!  wtf, php 5.1?
#define MONGO_METHOD_BASE(classname, name) zif_##classname##_##name
#endif /* ZEND_MODULE_API_NO >= 20060613 */

#define MONGO_METHOD_HELPER(classname, name, retval, thisptr, num, param) \
  PUSH_PARAM(param); PUSH_PARAM((void*)num);				\
  PUSH_EO_PARAM();							\
  MONGO_METHOD_BASE(classname, name)(num, retval, NULL, thisptr, 0 TSRMLS_CC); \
  POP_EO_PARAM();			\
  POP_PARAM(); POP_PARAM();

/* push parameters, call function, pop parameters */
#define MONGO_METHOD(classname, name, retval, thisptr)			\
  MONGO_METHOD_BASE(classname, name)(0, retval, NULL, thisptr, 0 TSRMLS_CC);

#define MONGO_METHOD1(classname, name, retval, thisptr, param1)		\
  MONGO_METHOD_HELPER(classname, name, retval, thisptr, 1, param1);

#define MONGO_METHOD2(classname, name, retval, thisptr, param1, param2)	\
  PUSH_PARAM(param1);							\
  MONGO_METHOD_HELPER(classname, name, retval, thisptr, 2, param2);	\
  POP_PARAM();

#define MONGO_METHOD3(classname, name, retval, thisptr, param1, param2, param3) \
  PUSH_PARAM(param1); PUSH_PARAM(param2);				\
  MONGO_METHOD_HELPER(classname, name, retval, thisptr, 3, param3);	\
  POP_PARAM(); POP_PARAM();

#define MONGO_METHOD4(classname, name, retval, thisptr, param1, param2, param3, param4) \
  PUSH_PARAM(param1); PUSH_PARAM(param2); PUSH_PARAM(param3);		\
  MONGO_METHOD_HELPER(classname, name, retval, thisptr, 4, param4);	\
  POP_PARAM(); POP_PARAM(); POP_PARAM();

#define MONGO_METHOD5(classname, name, retval, thisptr, param1, param2, param3, param4, param5) \
  PUSH_PARAM(param1); PUSH_PARAM(param2); PUSH_PARAM(param3); PUSH_PARAM(param4); \
  MONGO_METHOD_HELPER(classname, name, retval, thisptr, 5, param5);	\
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

#define MONGO_CMD(retval, thisptr) MONGO_METHOD1(MongoDB, command, retval, thisptr, data)


#define HASH_P(a) (Z_TYPE_P(a) == IS_ARRAY ? Z_ARRVAL_P(a) : Z_OBJPROP_P(a))
#define HASH_PP(a) (Z_TYPE_PP(a) == IS_ARRAY ? Z_ARRVAL_PP(a) : Z_OBJPROP_PP(a))

#define IS_SCALAR_P(a) (Z_TYPE_P(a) != IS_ARRAY && Z_TYPE_P(a) != IS_OBJECT)
#define IS_SCALAR_PP(a) (Z_TYPE_PP(a) != IS_ARRAY && Z_TYPE_PP(a) != IS_OBJECT)

#define php_mongo_obj_new(mongo_obj)                    \
  zend_object_value retval;                             \
  mongo_obj *intern;                                    \
  zval *tmp;                                            \
                                                        \
  intern = (mongo_obj*)emalloc(sizeof(mongo_obj));               \
  memset(intern, 0, sizeof(mongo_obj));                          \
                                                                 \
  zend_object_std_init(&intern->std, class_type TSRMLS_CC);      \
  zend_hash_copy(intern->std.properties,                         \
     &class_type->default_properties,                            \
     (copy_ctor_func_t) zval_add_ref,                            \
     (void *) &tmp,                                              \
     sizeof(zval *));                                            \
                                                                 \
  retval.handle = zend_objects_store_put(intern,                 \
     (zend_objects_store_dtor_t) zend_objects_destroy_object,    \
     php_##mongo_obj##_free, NULL TSRMLS_CC);                    \
  retval.handlers = &mongo_default_handlers;                     \
                                                                 \
  return retval;



typedef struct _mongo_server {
  char *host;
  int port;
  int socket;
  int connected;
} mongo_server;

typedef struct _mongo_server_set {
  int num;
  mongo_server **server;

  /*
   * this is the resource id if this is a persistent connection, so that we can
   * delete it from the list of persistent connections
   */
  int rsrc;

  // if num is greater than -1, master keeps track of the master connection
  int master;
} mongo_server_set;

typedef struct {
  zend_object std;

  // ts keeps track of the last time we tried to connect, so we don't try to
  // reconnect a zillion times in three seconds.
  int ts;
  int timeout;

  int persist;

  mongo_server_set *server_set;

  zval *db;
  zval *username;
  zval *password;

} mongo_link;

#define MONGO_LINK 0
#define MONGO_CURSOR 1

int php_mongo_free_cursor_le(void*, int TSRMLS_DC);


typedef struct {
  int length;
  int request_id;
  int response_to;
  int op;
} mongo_msg_header;

typedef struct {
  char *start;
  char *pos;
  char *end;
} buffer;

#define CREATE_MSG_HEADER(rid, rto, opcode)     \
  header.length = 0;                            \
  header.request_id = rid;                      \
  header.response_to = rto;                     \
  header.op = opcode;

#define CREATE_RESPONSE_HEADER(buf, ns, rto, opcode)    \
  CREATE_MSG_HEADER(MonGlo(request_id)++, rto, opcode); \
  APPEND_HEADER_NS(buf, ns, 0);

#define CREATE_HEADER_WITH_OPTS(buf, ns, opcode, opts)  \
  CREATE_MSG_HEADER(MonGlo(request_id)++, 0, opcode);   \
  APPEND_HEADER_NS(buf, ns, opts);

#define CREATE_HEADER(buf, ns, opcode)          \
  CREATE_RESPONSE_HEADER(buf, ns, 0, opcode);                    


#define APPEND_HEADER(buf, opts) buf.pos += INT_32;     \
  php_mongo_serialize_int(&buf, header.request_id);     \
  php_mongo_serialize_int(&buf, header.response_to);    \
  php_mongo_serialize_int(&buf, header.op);             \
  php_mongo_serialize_int(&buf, opts);                                


#define APPEND_HEADER_NS(buf, ns, opts)                         \
  APPEND_HEADER(buf, opts);                                     \
  php_mongo_serialize_ns(&buf, ns TSRMLS_CC);


#define MONGO_CHECK_INITIALIZED(member, class_name)                     \
  if (!(member)) {                                                      \
    zend_throw_exception(mongo_ce_Exception, "The " #class_name " object has not been correctly initialized by its constructor", 0 TSRMLS_CC); \
    RETURN_FALSE;                                                       \
  }

#define MONGO_CHECK_INITIALIZED_STRING(member, class_name)              \
  if (!(member)) {                                                      \
    zend_throw_exception(mongo_ce_Exception, "The " #class_name " object has not been correctly initialized by its constructor", 0 TSRMLS_CC); \
    RETURN_STRING("", 1);                                               \
  }

#define REPLY_HEADER_LEN 36

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

  char special;
  int timeout;

  mongo_msg_header send;
  mongo_msg_header recv;

  // response fields
  int flag;
  int64_t cursor_id;
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

/*
 * unfortunately, cursors can be freed before or after link is destroyed, so 
 * we can't actually depend on having a link to the database.  So, we're 
 * going to keep a separate list of link ids associated with cursor ids.
 *
 * When a cursor is to be freed, we try to find this cursor in the list.  If 
 * it's there, kill it.  If not, the db connection is probably already dead.
 * 
 * When a connection is killed, we sweep through the list and kill all the
 * cursors for that link.
 */
typedef struct _cursor_node {
  mongo_cursor *cursor;

  struct _cursor_node *next;
  struct _cursor_node *prev;
} cursor_node;
void php_mongo_free_cursor_node(cursor_node*, list_entry*);

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

#define CREATE_BUF(buf, size)                   \
  buf.start = (char*)emalloc(size);             \
  buf.pos = buf.start;                          \
  buf.end = buf.start + size;


PHP_MINIT_FUNCTION(mongo);
PHP_MSHUTDOWN_FUNCTION(mongo);
PHP_RINIT_FUNCTION(mongo);
PHP_MINFO_FUNCTION(mongo);

/*
 * serialization functions
 */
PHP_FUNCTION(bson_encode);
PHP_FUNCTION(bson_decode);

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
PHP_METHOD(Mongo, __get);
PHP_METHOD(Mongo, selectDB);
PHP_METHOD(Mongo, selectCollection);
PHP_METHOD(Mongo, dropDB);
PHP_METHOD(Mongo, lastError);
PHP_METHOD(Mongo, prevError);
PHP_METHOD(Mongo, resetError);
PHP_METHOD(Mongo, forceError);
PHP_METHOD(Mongo, close);
PHP_METHOD(Mongo, listDBs);

int php_mongo_create_le(mongo_cursor *cursor TSRMLS_DC);

/*
 * Mutex macros
 */

#ifdef WIN32
#define CHECK_LOCK_ERR                          \
  if (ret == WAIT_TIMEOUT) {                    \
    continue;                                   \
  }                                             \
  else {                                        \
    return ret;                                 \
  }
#define LOCK {                                  \
    int ret = -1;                               \
    int tries = 0;                              \
                                                \
    while (tries++ < 3 && ret != 0) {                     \
      ret = WaitForSingleObject(cursor_mutex, INFINITE);  \
      CHECK_LOCK_ERR                                      \
    }                                                 \
  }
#define UNLOCK {                                    \
    int ret = -1;                                   \
    int tries = 0;                                  \
                                                    \
    while (tries++ < 3 && ret != 0) {               \
      ret = ReleaseMutex(cursor_mutex);              \
      CHECK_LOCK_ERR                                 \
    }                                            \
  }
#else
#define CHECK_LOCK_ERR if (ret == -1) {         \
  if (errno == EAGAIN || errno == EBUSY) {      \
    continue;                                   \
  }                                             \
  else {                                        \
    return ret;                                 \
  }                                             \
}
#define LOCK {                                  \
  int ret = -1;                                 \
  int tries = 0;                                \
                                                \
  while (tries++ < 3 && ret != 0) {             \
    ret = pthread_mutex_lock(&cursor_mutex);              \
    CHECK_LOCK_ERR                                        \
  }                                                       \
}
#define UNLOCK {                                    \
    int ret = -1;                                   \
    int tries = 0;                                  \
                                                    \
    while (tries++ < 3 && ret != 0) {               \
      ret = pthread_mutex_unlock(&cursor_mutex);    \
      CHECK_LOCK_ERR                                 \
    }                                            \
  }
#endif


/*
 * Internal functions
 */
void mongo_do_up_connect_caller(INTERNAL_FUNCTION_PARAMETERS);
void mongo_do_connect_caller(INTERNAL_FUNCTION_PARAMETERS, zval *username, zval *password);
int mongo_say(mongo_link*, buffer*, zval* TSRMLS_DC);
int mongo_hear(mongo_link*, void*, int TSRMLS_DC);
int php_mongo_get_reply(mongo_cursor*, zval* TSRMLS_DC);

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
void mongo_init_MongoTimestamp(TSRMLS_D);

ZEND_BEGIN_MODULE_GLOBALS(mongo)
int auto_reconnect; 
int allow_persistent; 
char *default_host; 
int default_port;
int request_id; 
int chunk_size;
// $ alternative
char *cmd_char;
int utf8;

// _id generation helpers
int inc, pid, machine;

// timestamp generation helper
int ts_inc;
char *errmsg;
int response_num;
ZEND_END_MODULE_GLOBALS(mongo) 

#ifdef ZTS
#include <TSRM.h>
# define MonGlo(v) TSRMG(mongo_globals_id, zend_mongo_globals *, v)
#else
# define MonGlo(v) (mongo_globals.v)
#endif 

extern zend_module_entry mongo_module_entry;
#define phpext_mongo_ptr &mongo_module_entry

#endif
