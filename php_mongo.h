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


#ifndef PHP_MONGO_H
#define PHP_MONGO_H 1

#define PHP_MONGO_VERSION "1.3.0dev"
#define PHP_MONGO_EXTNAME "mongo"

// resource names
#define PHP_CONNECTION_RES_NAME "mongo connection"
#define PHP_SERVER_RES_NAME "mongo server info"
#define PHP_CURSOR_LIST_RES_NAME "cursor list"

#ifdef WIN32
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#endif

#ifndef Z_ADDREF_P
#  define Z_ADDREF_P(pz)                (pz)->refcount++
#endif

#ifndef Z_DELREF_P
#  define Z_DELREF_P(pz)                (pz)->refcount--
#endif


#define INT_32 4
#define INT_64 8
#define DOUBLE_64 8
#define BYTE_8 1

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
#define DEFAULT_CHUNK_SIZE (256*1024)

#define PHP_MONGO_DEFAULT_TIMEOUT 10000

// if _id field should be added
#define PREP 1
#define NO_PREP 0

#define NOISY 0
#define QUIET 1

// duplicate strings
#define DUP 1
#define NO_DUP 0

#define PERSIST 1
#define NO_PERSIST 0

#define FLAGS 0

#define LAST_ERROR 0
#define PREV_ERROR 1
#define RESET_ERROR 2
#define FORCE_ERROR 3

#if PHP_VERSION_ID > 50300
# define MONGO_ARGINFO_STATIC
#else
# define MONGO_ARGINFO_STATIC static
#endif

#if ZEND_MODULE_API_NO >= 20090115
# define PUSH_PARAM(arg) zend_vm_stack_push(arg TSRMLS_CC)
# define POP_PARAM() (void)zend_vm_stack_pop(TSRMLS_C)
# define PUSH_EO_PARAM()
# define POP_EO_PARAM()
#else
# define PUSH_PARAM(arg) zend_ptr_stack_push(&EG(argument_stack), arg)
# define POP_PARAM() (void)zend_ptr_stack_pop(&EG(argument_stack))
# define PUSH_EO_PARAM() zend_ptr_stack_push(&EG(argument_stack), NULL)
# define POP_EO_PARAM() (void)zend_ptr_stack_pop(&EG(argument_stack))
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

// TODO: this should be expanded to handle long_as_object being set
#define Z_NUMVAL_P(variable, value)                                     \
  ((Z_TYPE_P(variable) == IS_LONG && Z_LVAL_P(variable) == value) ||    \
   (Z_TYPE_P(variable) == IS_DOUBLE && Z_DVAL_P(variable) == value))
#define Z_NUMVAL_PP(variable, value)                                    \
  ((Z_TYPE_PP(variable) == IS_LONG && Z_LVAL_PP(variable) == value) ||  \
   (Z_TYPE_PP(variable) == IS_DOUBLE && Z_DVAL_PP(variable) == value))

#if ZEND_MODULE_API_NO >= 20100525
#define init_properties(intern) object_properties_init(&intern->std, class_type)
#else
#define init_properties(intern) zend_hash_copy(intern->std.properties, \
    &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,  \
    (void *) &tmp, sizeof(zval *))
#endif

#define php_mongo_obj_new(mongo_obj)                    \
  zend_object_value retval;                             \
  mongo_obj *intern;                                    \
  zval *tmp;                                            \
                                                        \
  intern = (mongo_obj*)emalloc(sizeof(mongo_obj));               \
  memset(intern, 0, sizeof(mongo_obj));                          \
                                                                 \
  zend_object_std_init(&intern->std, class_type TSRMLS_CC);      \
  init_properties(intern);                                       \
                                                                 \
  retval.handle = zend_objects_store_put(intern,                 \
     (zend_objects_store_dtor_t) zend_objects_destroy_object,    \
     php_##mongo_obj##_free, NULL TSRMLS_CC);                    \
  retval.handlers = &mongo_default_handlers;                     \
                                                                 \
  return retval;

#define RS_PRIMARY 1
#define RS_SECONDARY 2

typedef struct _mongo_server {
  int socket;
  int connected;
  pid_t owner;
  int port;
  char *host;
  char *label;

  char *username;
  char *password;
  char *db;

  struct _mongo_server *next;
  // list of handed-out sockets for this address
  struct _mongo_server *next_in_pool;
} mongo_server;

typedef struct _mongo_server_set {
  int num;
  // the last time we repopulated the server list
  int server_ts;
  // the last time we updated the hosts hash
  int ts;

  mongo_server *server;

  // if num is greater than -1, master keeps track of the master connection,
  // otherwise it points to "server"
  mongo_server *master;
} mongo_server_set;

typedef struct {
  zend_object std;

  int timeout;
  // if this is a replica set

  mongo_server_set *server_set;

  // slave to send reads to
  mongo_server *slave;

  // if this connection should distribute reads to slaves
  zend_bool slave_okay;
  char *username;
  char *password;
  char *db;
  char *rs;
} mongo_link;

#define MONGO_SERVER 0
#define MONGO_CURSOR 1


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


#define APPEND_HEADER(buf, opts) buf->pos += INT_32;     \
  php_mongo_serialize_int(buf, header.request_id);     \
  php_mongo_serialize_int(buf, header.response_to);    \
  php_mongo_serialize_int(buf, header.op);             \
  php_mongo_serialize_int(buf, opts);


#define APPEND_HEADER_NS(buf, ns, opts)                         \
  APPEND_HEADER(buf, opts);                                     \
  php_mongo_serialize_ns(buf, ns TSRMLS_CC);


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

#define PHP_MONGO_GET_LINK(obj)                                         \
  link = (mongo_link*)zend_object_store_get_object((obj) TSRMLS_CC);    \
  MONGO_CHECK_INITIALIZED(link->server_set, Mongo);

#define PHP_MONGO_GET_DB(obj)                                           \
  db = (mongo_db*)zend_object_store_get_object((obj) TSRMLS_CC);        \
  MONGO_CHECK_INITIALIZED(db->name, MongoDB);

#define PHP_MONGO_GET_COLLECTION(obj)                                   \
  c = (mongo_collection*)zend_object_store_get_object((obj) TSRMLS_CC); \
  MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

#define PHP_MONGO_GET_CURSOR(obj)                                       \
  cursor = (mongo_cursor*)zend_object_store_get_object((obj) TSRMLS_CC); \
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

#define PHP_MONGO_CHECK_EXCEPTION() if (EG(exception)) { return; }
#define PHP_MONGO_CHECK_EXCEPTION1(arg1)                        \
  if (EG(exception)) {                                          \
    zval_ptr_dtor(arg1);                                        \
    return;                                                     \
  }
#define PHP_MONGO_CHECK_EXCEPTION2(arg1, arg2)                  \
  if (EG(exception)) {                                          \
    zval_ptr_dtor(arg1);                                        \
    zval_ptr_dtor(arg2);                                        \
    return;                                                     \
  }
#define PHP_MONGO_CHECK_EXCEPTION3(arg1, arg2, arg3)            \
  if (EG(exception)) {                                          \
    zval_ptr_dtor(arg1);                                        \
    zval_ptr_dtor(arg2);                                        \
    zval_ptr_dtor(arg3);                                        \
    return;                                                     \
  }
#define PHP_MONGO_CHECK_EXCEPTION4(arg1, arg2, arg3, arg4)      \
  if (EG(exception)) {                                          \
    zval_ptr_dtor(arg1);                                        \
    zval_ptr_dtor(arg2);                                        \
    zval_ptr_dtor(arg3);                                        \
    zval_ptr_dtor(arg4);                                        \
    return;                                                     \
  }

#define PHP_MONGO_SERIALIZE_KEY(type)                           \
  php_mongo_set_type(buf, type);                                \
  php_mongo_serialize_key(buf, name, name_len, prep TSRMLS_CC); \
  if (EG(exception)) {                                          \
    return ZEND_HASH_APPLY_STOP;                                \
  }

#define SEND_MSG                                                \
  MAKE_STD_ZVAL(errmsg);                                        \
  ZVAL_NULL(errmsg);                                            \
                                                                \
  if (is_safe_op(options TSRMLS_CC)) {                          \
    zval *cursor = append_getlasterror(getThis(), &buf, options TSRMLS_CC); \
    if (cursor) {                                               \
      safe_op(server, cursor, &buf, return_value TSRMLS_CC);    \
    }                                                           \
    else {                                                      \
      RETVAL_FALSE;                                             \
    }                                                           \
  }                                                             \
  else if (mongo_say(server, &buf, errmsg TSRMLS_CC) == FAILURE) {\
    RETVAL_FALSE;                                               \
  }                                                             \
  else {                                                        \
    RETVAL_TRUE;                                                \
  }                                                             \
  zval_ptr_dtor(&errmsg);


#define GET_OPTIONS                                                     \
  timeout_p = zend_read_static_property(mongo_ce_Cursor, "timeout", strlen("timeout"), NOISY TSRMLS_CC); \
  timeout = Z_LVAL_P(timeout_p);                                        \
                                                                        \
  if (options && !IS_SCALAR_P(options)) {                               \
    zval **safe_pp, **fsync_pp, **timeout_pp;                           \
                                                                        \
    if (SUCCESS == zend_hash_find(HASH_P(options), "safe", strlen("safe")+1, (void**)&safe_pp)) { \
      if (Z_TYPE_PP(safe_pp) == IS_STRING) {                            \
        safe_str = Z_STRVAL_PP(safe_pp);                                \
      }                                                                 \
      else {                                                            \
        safe = Z_LVAL_PP(safe_pp);                                      \
      }                                                                 \
    }                                                                   \
    if (SUCCESS == zend_hash_find(HASH_P(options), "fsync", strlen("fsync")+1, (void**)&fsync_pp)) { \
      fsync = Z_BVAL_PP(fsync_pp);                                      \
      if (fsync && !safe) {                                             \
        safe = 1;                                                       \
      }                                                                 \
    }                                                                   \
    if (SUCCESS == zend_hash_find(HASH_P(options), "timeout", strlen("timeout")+1, (void**)&timeout_pp)) {\
      timeout = Z_LVAL_PP(timeout_pp);                                  \
    }                                                                   \
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
  int batch_size;
  int skip;
  int opts;

  char special;
  int timeout;

  mongo_msg_header send;
  mongo_msg_header recv;

  // response fields
  int flag;
  int start;
  // number of results used
  int at;
  // number results returned
  int num;
  // results
  buffer buf;

  // cursor_id indicates if there are more results to fetch.  If cursor_id is 0,
  // the cursor is "dead."  If cursor_id != 0, server is set to the server that
  // was queried, so a get_more doesn't try to fetch results from the wrong
  // server.  server just points to a member of link, so it should never need to
  // be freed.
  int64_t cursor_id;
  mongo_server *server;

  zend_bool started_iterating;
  zend_bool persist;

  zval *current;
  int retry;

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
  int64_t cursor_id;
  int socket;

  struct _cursor_node *next;
  struct _cursor_node *prev;
} cursor_node;

typedef struct {
  zend_object std;
  char *id;
} mongo_id;


typedef struct {
  zend_object std;
  zval *link;
  zval *name;

  zend_bool slave_okay;
} mongo_db;

typedef struct {
  zend_object std;

  // parent database
  zval *parent;
  zval *link;

  // names
  zval *name;
  zval *ns;

  zend_bool slave_okay;
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
 * Mutex macros
 */

#ifdef WIN32
#define LOCK(lk) {                              \
    int ret = -1;                               \
    int tries = 0;                              \
                                                \
    while (tries++ < 3 && ret != 0) {                 \
      ret = WaitForSingleObject(lk##_mutex, 5000);    \
      if (ret != 0) {                                 \
        if (ret == WAIT_TIMEOUT) {                    \
          continue;                                   \
        }                                             \
        else {                                                          \
          break;                                                        \
        }                                                               \
      }                                                                 \
    }                                                                   \
  }
#define UNLOCK(lk) ReleaseMutex(lk##_mutex);
#else
#define LOCK(lk) pthread_mutex_lock(&lk##_mutex);
#define UNLOCK(lk) pthread_mutex_unlock(&lk##_mutex);
#endif




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
void mongo_init_MongoInt32(TSRMLS_D);
void mongo_init_MongoInt64(TSRMLS_D);

ZEND_BEGIN_MODULE_GLOBALS(mongo)
// php.ini options
// these must be IN THE SAME ORDER as mongo.c lists them
int auto_reconnect;
int allow_persistent;
char *default_host;
int default_port;
int request_id;
int chunk_size;
// $ alternative
char *cmd_char;
int utf8;
int native_long;
int long_as_object;
int allow_empty_keys;
int no_id;

// _id generation helpers
int inc, pid, machine;

// timestamp generation helper
int ts_inc;
char *errmsg;
int response_num;
int max_send_size;
int pool_size;

	long log_level;
	long log_module;

	long ping_interval;
	long is_master_interval;
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

/*
 * Error codes
 *
 * MongoException:
 * 0: The <class> object has not been correctly initialized by its constructor
 * 1: zero-length keys are not allowed, did you use $ with double quotes?
 * 2: '.' not allowed in key: <key>
 * 3: insert too large: <size>, max: 16000000
 * 4: no elements in doc
 * 5: size of BSON doc is <size> bytes, max 4MB
 * 6: no documents given
 * 7: MongoCollection::group takes an array, object, or MongoCode key
 * 8: field names must be strings
 * 9: invalid regex
 * 10: MongoDBRef::get: $ref field must be a string
 * 11: MongoDBRef::get: $db field must be a string
 * 12: non-utf8 string: <str>
 * 13: mutex error: <err>
 * 14: index name too long: <len>, max <max> characters
 * 15: Reading from slaves won't work without using the replicaSet option on connect
 * 16: No server found for reads
 * 17: The MongoCollection object has not been correctly initialized by its constructor
 *
 * MongoConnectionException:
 * 0: connection to <host> failed: <errmsg>
 * 1: no server name given
 * 2: can't use slaveOkay without replicaSet
 * 3: could not store persistent link
 * 4: pass in an identifying string to get a persistent connection
 * 5: failed to get primary or secondary
 * 10: failed to get host from <substr> of <str>
 * 11: failed to get port from <substr> of <str>
 * 12: lost db connection
 *
 * MongoCursorException:
 * 0: cannot modify cursor after beginning iteration
 * 1: get more: send error (C error string)
 * 2: get more: cursor not found
 * 3: cursor->buf.pos is null
 * 4: couldn't get response header
 * 5: no db response
 * 6: bad response length: <len>, max: <len>, did the db assert?
 * 7: incomplete header
 * 8: incomplete response
 * 9: couldn't find a response
 * 10: error getting socket
 * 11: couldn't find reply, please try again
 * 12: [WSA ]error getting database response: <err>
 * 13: Timeout error (C error)
 * 14: couldn't send query: <err>
 * 15: couldn't get sock for safe op
 * 16: couldn't send safe op
 * 17: exceptional condition on socket
 * 18: Trying to get more, but cannot find server
 * 19: max number of retries exhausted, couldn't send query
 * various: database error
 */

