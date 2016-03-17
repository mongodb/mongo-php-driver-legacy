/**
 *  Copyright 2009-2014 MongoDB, Inc.
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

#define PHP_MONGO_VERSION "1.7.0dev"
#define PHP_MONGO_EXTNAME "mongo"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(_MSC_VER)
# define strtoll(s, f, b) _atoi64(s)
#elif !defined(HAVE_STRTOLL)
# if defined(HAVE_ATOLL)
#  define strtoll(s, f, b) atoll(s)
# else
#  define strtoll(s, f, b) strtol(s, f, b)
# endif
#endif


#include "mcon/types.h"
#include "mcon/read_preference.h"

#ifndef zend_parse_parameters_none
# define zend_parse_parameters_none() \
	zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")
#endif

#ifndef Z_UNSET_ISREF_P
# define Z_UNSET_ISREF_P(pz)      pz->is_ref = 0
#endif

#ifdef WIN32
# ifndef int64_t
typedef __int64 int64_t;
# endif
#endif

#ifndef Z_ADDREF_P
# define Z_ADDREF_P(pz)                (pz)->refcount++
#endif

#ifndef Z_ADDREF_PP
# define Z_ADDREF_PP(ppz)               Z_ADDREF_P(*(ppz))
#endif

#ifndef Z_DELREF_P
# define Z_DELREF_P(pz)                (pz)->refcount--
#endif

#ifndef Z_SET_REFCOUNT_P
# define Z_SET_REFCOUNT_P(pz, rc)      (pz)->refcount = (rc)
#endif

#define INT_32 4
#define INT_64 8
#define DOUBLE_64 8
#define BYTE_8 1

/* db ops */
#define OP_REPLY 1
#define OP_MSG 1000
#define OP_UPDATE 2001
#define OP_INSERT 2002
#define OP_GET_BY_OID 2003
#define OP_QUERY 2004
#define OP_GET_MORE 2005
#define OP_DELETE 2006
#define OP_KILL_CURSORS 2007

#define REPLY_HEADER_SIZE 36

#define INITIAL_BUF_SIZE 4096
#define DEFAULT_CHUNK_SIZE (255*1024)
#define DEFAULT_CHUNK_SIZE_S "261120"

#define PHP_MONGO_DEFAULT_WTIMEOUT 10000
#define PHP_MONGO_STATIC_CURSOR_TIMEOUT_NOT_SET_INITIALIZER -2

#define PHP_MONGO_COLLECTION_DOES_NOT_EXIST 26

/* if _id field should be added */
#define PREP 1
#define NO_PREP 0

#define NOISY 0
#define QUIET 1

/* duplicate strings */
#define DUP 1
#define NO_DUP 0

#define PUSH_PARAM(arg) zend_vm_stack_push(arg TSRMLS_CC)
#define POP_PARAM() (void)zend_vm_stack_pop(TSRMLS_C)
#define PUSH_EO_PARAM()
#define POP_EO_PARAM()

#if PHP_VERSION_ID < 50307
# define PHP_FE_END         { NULL, NULL, NULL, 0, 0 }
#endif

#define MUST_BE_ARRAY_OR_OBJECT(num, arg) do { \
	if (arg && !(Z_TYPE_P(arg) == IS_ARRAY || Z_TYPE_P(arg) == IS_OBJECT)) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects parameter %d to be an array or object, %s given", num, zend_get_type_by_const(Z_TYPE_P(arg))); \
		RETURN_NULL(); \
	} \
} while(0);

#define MONGO_METHOD_BASE(classname, name) zim_##classname##_##name

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
	ZEND_VM_STACK_GROW_IF_NEEDED(2); \
	MONGO_METHOD_HELPER(classname, name, retval, thisptr, 1, param1);

#define MONGO_METHOD2(classname, name, retval, thisptr, param1, param2)	\
	ZEND_VM_STACK_GROW_IF_NEEDED(3); \
	PUSH_PARAM(param1);							\
	MONGO_METHOD_HELPER(classname, name, retval, thisptr, 2, param2);	\
	POP_PARAM();

#define MONGO_METHOD3(classname, name, retval, thisptr, param1, param2, param3) \
	ZEND_VM_STACK_GROW_IF_NEEDED(4); \
	PUSH_PARAM(param1); PUSH_PARAM(param2);				\
	MONGO_METHOD_HELPER(classname, name, retval, thisptr, 3, param3);	\
	POP_PARAM(); POP_PARAM();

#define MONGO_METHOD4(classname, name, retval, thisptr, param1, param2, param3, param4) \
	ZEND_VM_STACK_GROW_IF_NEEDED(5); \
	PUSH_PARAM(param1); PUSH_PARAM(param2); PUSH_PARAM(param3);		\
	MONGO_METHOD_HELPER(classname, name, retval, thisptr, 4, param4);	\
	POP_PARAM(); POP_PARAM(); POP_PARAM();

#define MONGO_METHOD5(classname, name, retval, thisptr, param1, param2, param3, param4, param5) \
	ZEND_VM_STACK_GROW_IF_NEEDED(6); \
	PUSH_PARAM(param1); PUSH_PARAM(param2); PUSH_PARAM(param3); PUSH_PARAM(param4); \
	MONGO_METHOD_HELPER(classname, name, retval, thisptr, 5, param5);	\
	POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

#define HASH_P(a) (Z_TYPE_P(a) == IS_ARRAY ? Z_ARRVAL_P(a) : Z_OBJPROP_P(a))
#define HASH_PP(a) (Z_TYPE_PP(a) == IS_ARRAY ? Z_ARRVAL_PP(a) : Z_OBJPROP_PP(a))

#define IS_SCALAR_P(a) (Z_TYPE_P(a) == IS_NULL || Z_TYPE_P(a) == IS_LONG || Z_TYPE_P(a) == IS_DOUBLE || Z_TYPE_P(a) == IS_BOOL || Z_TYPE_P(a) == IS_STRING)
#define IS_SCALAR_PP(a) IS_SCALAR_P(*a)
#define IS_ARRAY_OR_OBJECT_P(a) (Z_TYPE_P(a) == IS_ARRAY || Z_TYPE_P(a) == IS_OBJECT)

#if PHP_VERSION_ID >= 50400
# define init_properties(intern) object_properties_init(&intern->std, class_type)
#else
# define init_properties(intern) {                                     \
	zval *tmp;                                                         \
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *)); \
}
#endif

#define PHP_MONGO_OBJ_NEW(mongo_obj)                    \
	zend_object_value retval;                           \
	mongo_obj *intern;                                  \
	                                                    \
	intern = (mongo_obj*)emalloc(sizeof(mongo_obj));               \
	memset(intern, 0, sizeof(mongo_obj));                          \
	                                                               \
	zend_object_std_init(&intern->std, class_type TSRMLS_CC);      \
	init_properties(intern);                                       \
	                                                               \
	retval.handle = zend_objects_store_put(intern,(zend_objects_store_dtor_t) zend_objects_destroy_object, php_##mongo_obj##_free, NULL TSRMLS_CC); \
	retval.handlers = &mongo_default_handlers;                     \
	                                                               \
	return retval;

zend_object_value php_mongo_type_object_new(zend_class_entry *class_type TSRMLS_DC);
void php_mongo_type_object_free(void *object TSRMLS_DC);

#if PHP_VERSION_ID >= 50400
void mongo_write_property(zval *object, zval *member, zval *value, const zend_literal *key TSRMLS_DC);
#else
void mongo_write_property(zval *object, zval *member, zval *value TSRMLS_DC);
#endif

#if PHP_VERSION_ID >= 50400
zval *mongo_read_property(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC);
#else
zval *mongo_read_property(zval *object, zval *member, int type TSRMLS_DC);
#endif


/* Used in our _write_property() handler to mark properties are userland Read Only */
#define MONGO_ACC_READ_ONLY 0x10000000

typedef struct {
	zend_object std;
} mongo_type_object;

typedef struct {
	zend_object std;

	mongo_con_manager *manager; /* Contains a link to the manager */
	mongo_servers     *servers;
} mongoclient;

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
} mongo_buffer;

#define CREATE_MSG_HEADER(rid, rto, opcode) \
	header.length = 0; \
	header.request_id = rid; \
	header.response_to = rto; \
	header.op = opcode;

#define CREATE_RESPONSE_HEADER(buf, ns, rto, opcode) \
	CREATE_MSG_HEADER(MonGlo(request_id)++, rto, opcode); \
	APPEND_HEADER_NS(buf, ns, 0);

#define CREATE_HEADER_WITH_OPTS(buf, ns, opcode, opts) \
	CREATE_MSG_HEADER(MonGlo(request_id)++, 0, opcode); \
	APPEND_HEADER_NS(buf, ns, opts);

#define CREATE_HEADER(buf, ns, opcode) \
	CREATE_RESPONSE_HEADER(buf, ns, 0, opcode);

#define APPEND_HEADER(buf, opts) buf->pos += INT_32; \
	php_mongo_serialize_int(buf, header.request_id); \
	php_mongo_serialize_int(buf, header.response_to); \
	php_mongo_serialize_int(buf, header.op); \
	php_mongo_serialize_int(buf, opts);


#define APPEND_HEADER_NS(buf, ns, opts) \
	APPEND_HEADER(buf, opts); \
	php_mongo_serialize_ns(buf, ns TSRMLS_CC);


#define MONGO_CHECK_INITIALIZED(member, class_name) \
	if (!(member)) { \
		zend_throw_exception(mongo_ce_Exception, "The " #class_name " object has not been correctly initialized by its constructor", 0 TSRMLS_CC); \
		RETURN_FALSE; \
	}

#define MONGO_CHECK_INITIALIZED_C(member, class_name) \
	if (!(member)) { \
		zend_throw_exception(mongo_ce_Exception, "The " #class_name " object has not been correctly initialized by its constructor", 0 TSRMLS_CC); \
		return FAILURE; \
	}

#define MONGO_CHECK_INITIALIZED_STRING(member, class_name) \
	if (!(member)) { \
		zend_throw_exception(mongo_ce_Exception, "The " #class_name " object has not been correctly initialized by its constructor", 0 TSRMLS_CC); \
		RETURN_STRING("", 1); \
	}

#define PHP_MONGO_GET_LINK(obj) \
	link = (mongoclient*)zend_object_store_get_object((obj) TSRMLS_CC); \
	MONGO_CHECK_INITIALIZED(link->servers, Mongo);

#define PHP_MONGO_GET_DB(obj) \
	db = (mongo_db*)zend_object_store_get_object((obj) TSRMLS_CC); \
	MONGO_CHECK_INITIALIZED(db->name, MongoDB);

#define PHP_MONGO_GET_COLLECTION(obj) \
	c = (mongo_collection*)zend_object_store_get_object((obj) TSRMLS_CC); \
	MONGO_CHECK_INITIALIZED(c->ns, MongoCollection);

#define PHP_MONGO_GET_CURSOR(obj) \
	cursor = (mongo_cursor*)zend_object_store_get_object((obj) TSRMLS_CC); \
	MONGO_CHECK_INITIALIZED(cursor->zmongoclient, MongoCursor);

#define PHP_MONGO_GET_MONGOCLIENT_FROM_CURSOR(cursor) \
	client = (mongoclient*)zend_object_store_get_object((cursor)->zmongoclient TSRMLS_CC);

#define PHP_MONGO_CHECK_EXCEPTION() if (EG(exception)) { return; }

#define PHP_MONGO_CHECK_EXCEPTION1(arg1) \
	if (EG(exception)) { \
		if (*arg1) { zval_ptr_dtor(arg1); } \
		return; \
	}

#define PHP_MONGO_CHECK_EXCEPTION2(arg1, arg2) \
	if (EG(exception)) { \
		if (*arg1) { zval_ptr_dtor(arg1); } \
		if (*arg2) { zval_ptr_dtor(arg2); } \
		return; \
	}

#define PHP_MONGO_CHECK_EXCEPTION3(arg1, arg2, arg3) \
	if (EG(exception)) { \
		if (*arg1) { zval_ptr_dtor(arg1); } \
		if (*arg2) { zval_ptr_dtor(arg2); } \
		if (*arg3) { zval_ptr_dtor(arg3); } \
		return; \
	}

#define PHP_MONGO_CHECK_EXCEPTION4(arg1, arg2, arg3, arg4) \
	if (EG(exception)) { \
		if (*arg1) { zval_ptr_dtor(arg1); } \
		if (*arg2) { zval_ptr_dtor(arg2); } \
		if (*arg3) { zval_ptr_dtor(arg3); } \
		if (*arg4) { zval_ptr_dtor(arg4); } \
		return; \
	}

#define PHP_MONGO_SERIALIZE_KEY(type) \
	php_mongo_set_type(buf, type); \
	php_mongo_serialize_key(buf, name, name_len, prep TSRMLS_CC); \
	if (EG(exception)) { \
		return ZEND_HASH_APPLY_STOP; \
	}


#define REPLY_HEADER_LEN 36

typedef struct {
	zend_object std;

	/* Connection */
	mongo_connection *connection;
	zval *zmongoclient;

	/* collection namespace */
	char *ns;

	/* fields to send */
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

	/* response fields */
	int flag;
	int start;
	/* number of results used */
	int at;
	/* number results returned */
	int num;
	/* results */
	mongo_buffer buf;

	/* cursor_id indicates if there are more results to fetch.  If cursor_id
	 * is 0, the cursor is "dead."  If cursor_id != 0, server is set to the
	 * server that was queried, so a get_more doesn't try to fetch results
	 * from the wrong server.  server just points to a member of link, so it
	 * should never need to be freed. */
	int64_t cursor_id;

	zend_bool started_iterating;
	zend_bool pre_created; /* Used for command cursors through createFromDocument */
	zend_bool persist;

	zval *current;
	int retry;

	mongo_read_preference read_pref;

	int dead;

	/* Options that deal with changes to what the cursor documents return. For
	 * example forcing longs to be returned as objects */
	int cursor_options;

	/* Fields that are only used for command cursors */
	zval *first_batch;     /* The first batch of results */
	int   first_batch_at;  /* The current cursor position in the first batch */
	int   first_batch_num; /* The number of entries in the first batch */
} mongo_cursor;

typedef mongo_cursor mongo_command_cursor;

typedef struct {
	zend_object std;
	char *id;
} mongo_id;


typedef struct {
	zend_object std;
	zval *link;
	zval *name;

	mongo_read_preference read_pref;
} mongo_db;

typedef struct {
	zend_object std;

	/* parent database */
	zval *parent;
	zval *link;

	/* names */
	zval *name;
	zval *ns;

	mongo_read_preference read_pref;
} mongo_collection;

#include "api/write.h"

typedef struct {
	zend_object              std;
	php_mongo_write_types    batch_type;
	zval                    *zcollection_object;
	php_mongo_batch         *batch;
	php_mongo_write_options  write_options;
	int                      total_items;
} mongo_write_batch_object;


#define BUF_REMAINING (buf->end-buf->pos)

#define CREATE_BUF(buf, size) \
	buf.start = (char*)emalloc(size); \
	buf.pos = buf.start; \
	buf.end = buf.start + size;

PHP_MINIT_FUNCTION(mongo);
PHP_MSHUTDOWN_FUNCTION(mongo);
PHP_RINIT_FUNCTION(mongo);
PHP_MINFO_FUNCTION(mongo);

/* Serialization functions */
PHP_FUNCTION(bson_encode);
PHP_FUNCTION(bson_decode);


void mongo_init_MongoDB(TSRMLS_D);
void mongo_init_MongoCollection(TSRMLS_D);

void mongo_init_MongoGridFS(TSRMLS_D);
void mongo_init_MongoGridFSFile(TSRMLS_D);
void mongo_init_MongoGridFSCursor(TSRMLS_D);

void mongo_init_MongoWriteBatch(TSRMLS_D);
void mongo_init_MongoInsertBatch(TSRMLS_D);
void mongo_init_MongoUpdateBatch(TSRMLS_D);
void mongo_init_MongoDeleteBatch(TSRMLS_D);

void mongo_init_MongoId(TSRMLS_D);
void mongo_init_MongoCode(TSRMLS_D);
void mongo_init_MongoRegex(TSRMLS_D);
void mongo_init_MongoDate(TSRMLS_D);
void mongo_init_MongoBinData(TSRMLS_D);
void mongo_init_MongoDBRef(TSRMLS_D);
void mongo_init_MongoTimestamp(TSRMLS_D);
void mongo_init_MongoInt32(TSRMLS_D);
void mongo_init_MongoInt64(TSRMLS_D);

/* Shared helper functions */
zval *php_mongo_make_tagsets(mongo_read_preference *rp);
void php_mongo_add_tagsets(zval *return_value, mongo_read_preference *rp);
int php_mongo_set_readpreference(mongo_read_preference *rp, char *read_preference, HashTable *tags TSRMLS_DC);
int php_mongo_trigger_error_on_command_failure(mongo_connection *connection, zval *document TSRMLS_DC);
int php_mongo_trigger_error_on_gle(mongo_connection *connection, zval *document TSRMLS_DC);

ZEND_BEGIN_MODULE_GLOBALS(mongo)
	/* php.ini options */
	char *default_host;
	long default_port;
	long request_id;
	long chunk_size;

	/* $ alternative */
	char *cmd_char;
	long native_long;
	long long_as_object;
	long allow_empty_keys;

	/* _id generation helpers */
	int inc, pid, machine;

	/* timestamp generation helper */
	long ts_inc;
	char *errmsg;

	long log_level;
	long log_module;
	zend_fcall_info log_callback_info;
	zend_fcall_info_cache log_callback_info_cache;

	long ping_interval;
	long ismaster_interval;

	mongo_con_manager *manager;
ZEND_END_MODULE_GLOBALS(mongo)

#ifdef ZTS
#include <TSRM.h>
# define MonGlo(v) TSRMG(mongo_globals_id, zend_mongo_globals *, v)
#else
# define MonGlo(v) (mongo_globals.v)
#endif

#define phpext_mongo_ptr &mongo_module_entry

extern zend_module_entry mongo_module_entry;

#endif

/*
 * Error codes
 *
 * TODO: Check and update those all 
 *
 * MongoException:
 * 0: The <class> object has not been correctly initialized by its constructor
 * 1: zero-length keys are not allowed, did you use $ with double quotes?
 * 2: characters not allowed in key: <key>
 * 3: insert too large: <size>, max: 16000000
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
 * 18: ID must be valid hex characters
 * 19: Invalid object ID
 * 20: Cannot run command count(): (error message from MongoDB)
 * 21: Namespace field is invalid.
 * 22: invalid index specification
 * 25: invalid RFC4122 UUID size
 * 26: Invalid type of "filter" option for collection enumeration method
 * 27: Invalid type of "filter" option "name" criteria for collection enumeration method on MongoDB <2.8
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
 * 2X: Parsing errors (unused)
 * 21: Empty option name or value
 * 22: Unknown connection string option
 * 23: Logical error (conflicting options)
 * 24: (unused)
 * 25: Option with no string key
 * 26: SSL support is only available when compiled against PHP Streams
 * 27: Driver options are only available when compiled against PHP Streams
 * 28: GSSAPI authentication mechanism is only available when compiled against PHP Streams
 * 29: Plain authentication mechanism is only available when compiled against PHP Streams
 * 31: Unknown failure doing io_stream_read.
 * 32: When the remote server closes the connection in io_stream_read.
 * 72: Could not retrieve connection
 *
 * MongoCursorTimeoutException:
 * 80: timeout exception
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
 * 20: something exceptional has happened, and the cursor is now dead
 * 21: Reading data for type %02x would exceed buffer for key "%s"
 * 21: invalid string length for key "%s"
 * 22: invalid binary length for key "%s"
 * 23: Can not natively represent the long %llu on this platform
 * 24: invalid code length for key "%s"
 * 25: invalid RFC4122 UUID size
 * 28: recv_header() (abs()) recv_data() stream handlers error (timeout)
 * 29: Unknown query/get_more failure
 * 30: Cursor command response does not have the expected structure
 * 32: Cursor command structure is invalid
 * 33: cannot iterate twice with command cursors created through createFromDocument
 * 34: Cursor structure is invalid
 * 35: Message size (%d) overflows valid message size (%d) php_mongo_api_get_reply()
 * 36: request/response mismatch: %d vs %d php_mongo_api_get_reply()
 * 37: Couldn't finish reading from network
 * 38: Reading document length would exceed buffer (%u bytes)
 * 39: Document length (%d bytes) should be at least 5 (i.e. empty document)
 * 40: Document length (%d bytes) exceeds buffer (%u bytes)
 * 41: string for key "%s" is not null-terminated
 * 42: Document length (%u bytes) is not equal to buffer (%u bytes)
 *
 * MongoGridFSException:
 * 0: 
 * 1: There is more data in the stored file than the meta data shows
 * 2: Invalid collection prefix (throws Exception, not MongoGridFSException)
 * 3: Could not open file for reading
 * 4: Filesize larger then we can handle
 * 5: Invalid filehandle for a resource
 * 6: Resource doesn't contain filehandle
 * 7: Error setting up file for reading
 * 8: Argument not a file stream or a filename string (throws Exception, not MongoGridFSException)
 * 9: Error reading file data
 * 10: Error reading from resource
 * 11: Can't find uploaded file
 * 12: tmp_name not found, upload probably failed
 * 13: tmp_name was not a valid filename
 * 14: Unable to determin file size
 * 15: Missing filename
 * 16: Could not open filename for writing
 * 17: Could not read chunk
 * 18: Failed creating file stream
 * 19: Could not find array key
 * 20: Chunk larger then chunksize
 * 21: Unexpected chunk format
 *
 * MongoResultException:
 * 1: Unknown error executing command (empty document returned)
 * 2: Command could not be executed for some reason (exception message tells why)
 * 1000+: MongoDB server codes
 *
 * MongoWriteConcernException:
 * 100: <server error message> (server returned no error code, only message)
 * 101: Unknown error occurred, did not get an error message or code
 * 102: Got write errors, but don't know how to parse them
 * 103: Missing 'ok' field in response, don't know what to do
 * 104: Got write errors, but don't know how to parse them
 * 105: Got write errors, but don't know how to parse them
 * (all other error codes are from mongod)
 *
 * MongoProtocolException:
 * 1: Current primary does not have a Write API support
 */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
