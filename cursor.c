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
#include <zend_interfaces.h>
#include <zend_exceptions.h>
#include "mcon/io.h"
#include "mcon/manager.h"

#ifdef WIN32
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#else
#include <unistd.h>
#include <pthread.h>
#endif
#include <math.h>

#include "php_mongo.h"
#include "bson.h"
#include "db.h"
#include "cursor.h"
#include "collection.h"
#include "mongo_types.h"
#include "util/log.h"

#if WIN32
HANDLE cursor_mutex;
#else
static pthread_mutex_t cursor_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Cursor flags */
#define CURSOR_FLAG_TAILABLE      2
#define CURSOR_FLAG_SLAVE_OKAY    4
#define CURSOR_FLAG_OPLOG_REPLAY  8 /* Don't use */
#define CURSOR_FLAG_NO_CURSOR_TO 16
#define CURSOR_FLAG_AWAIT_DATA   32
#define CURSOR_FLAG_EXHAUST      64 /* Not implemented */
#define CURSOR_FLAG_PARTIAL     128

// externs
extern zend_class_entry *mongo_ce_Id,
  *mongo_ce_Mongo,
  *mongo_ce_DB,
  *mongo_ce_Collection,
  *mongo_ce_Exception,
  *mongo_ce_ConnectionException,
  *mongo_ce_CursorTOException;

extern int le_pconnection,
  le_cursor_list;

extern zend_object_handlers mongo_default_handlers;

zend_class_entry *mongo_ce_CursorException;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

static zend_object_value php_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC);
static void make_special(mongo_cursor *);
static void kill_cursor(cursor_node *node, zend_rsrc_list_entry *le TSRMLS_DC);

zend_class_entry *mongo_ce_Cursor = NULL;

/*
 * Cursor related read/write functions
 */
/*
 * This method reads the message header for a database response
 * It returns failure or success and throws an exception on failure.
 *
 * Returns:
 * 0 on success
 * -1 on failure, but not critical enough to throw an exception
 * 1.. on failure, and throw an exception. The return value is the error code
 */
static signed int get_cursor_header(int sock, mongo_cursor *cursor, char **error_message TSRMLS_DC)
{
	int status = 0;
	int num_returned = 0;
	char buf[REPLY_HEADER_LEN];

	php_mongo_log(MLOG_IO, MLOG_FINE TSRMLS_CC, "getting cursor header");

	/* set a timeout */
	if (cursor->timeout && cursor->timeout > 0) {
		status = mongo_io_wait_with_timeout(sock, cursor->timeout, error_message);
		if (status != 0) {
			return status;
		}
	}

	status = recv(sock, buf, REPLY_HEADER_LEN, FLAGS);
	/* socket has been closed, retry */
	if (status == 0) {
		*error_message = strdup("socket has been closed, retry");
		return -1;
	} else if (status < INT_32*4) {
		*error_message = strdup("couldn't get response header");
		return 4;
	}

	/* switch the byte order, if necessary */
	cursor->recv.length = MONGO_32(*(int*)buf);

	/* make sure we're not getting crazy data */
	if (cursor->recv.length == 0) {
		*error_message = strdup("No response from the database");
		return 5;
	} else if (cursor->recv.length < REPLY_HEADER_SIZE) {
		*error_message = malloc(256);
		snprintf(*error_message, 256, "bad response length: %d, did the db assert?", cursor->recv.length);
		return 6;
	}

	cursor->recv.request_id  = MONGO_32(*(int*)(buf+INT_32));
	cursor->recv.response_to = MONGO_32(*(int*)(buf+INT_32*2));
	cursor->recv.op          = MONGO_32(*(int*)(buf+INT_32*3));
	cursor->flag             = MONGO_32(*(int*)(buf+INT_32*4));
	cursor->cursor_id        = MONGO_64(*(int64_t*)(buf+INT_32*5));
	cursor->start            = MONGO_32(*(int*)(buf+INT_32*5+INT_64));
	num_returned             = MONGO_32(*(int*)(buf+INT_32*6+INT_64));

	/* TODO: find out what this does */
	if (cursor->recv.response_to > MonGlo(response_num)) {
		MonGlo(response_num) = cursor->recv.response_to;
	}

	/* cursor->num is the total of the elements we've retrieved (elements
	 * already iterated through + elements in db response but not yet iterated
	 * through) */
	cursor->num += num_returned;

	/* create buf */
	cursor->recv.length -= REPLY_HEADER_LEN;

	return 0;
}

/* Reads a cursors body
 * Returns 0 on failure or an int indicating the number of bytes read */
static int get_cursor_body(int sock, mongo_cursor *cursor, char **error_message TSRMLS_DC)
{
	php_mongo_log(MLOG_IO, MLOG_FINE TSRMLS_CC, "getting cursor body");

	if (cursor->buf.start) {
		efree(cursor->buf.start);
	}

	cursor->buf.start = (char*)emalloc(cursor->recv.length);
	cursor->buf.end = cursor->buf.start + cursor->recv.length;
	cursor->buf.pos = cursor->buf.start;

	/* finish populating cursor */
	return mongo_io_recv_data(sock, cursor->buf.pos, cursor->recv.length, error_message);
}

/* Cursor helper function */
int php_mongo_get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC)
{
	int          sock;
	unsigned int status;
	char        *error_message = NULL;

	php_mongo_log(MLOG_IO, MLOG_FINE TSRMLS_CC, "getting reply");
	sock = cursor->connection->socket;

	status = get_cursor_header(sock, cursor, (char**) &error_message TSRMLS_CC);
	if (status == -1) {
		return FAILURE;
	}
	if (status > 0) {
		mongo_cursor_throw(cursor->connection, status TSRMLS_CC, error_message);
		return FAILURE;
	}

	/* Check that this is actually the response we want */
	if (cursor->send.request_id != cursor->recv.response_to) {
		php_mongo_log(MLOG_IO, MLOG_WARN TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);

		mongo_cursor_throw(cursor->connection, 9 TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);
		return FAILURE;
	}

	if (FAILURE == get_cursor_body(sock, cursor, (char **) &error_message TSRMLS_CC)) {
#ifdef WIN32
		mongo_cursor_throw(cursor->connection, 12 TSRMLS_CC, "WSA error getting database response %s (%d)", error_message, WSAGetLastError());
#else
		mongo_cursor_throw(cursor->connection, 12 TSRMLS_CC, "error getting database response %s (%d)", error_message, strerror(errno));
#endif
		return FAILURE;
	}

	/* If no catastrophic error has happened yet, we're fine, set errmsg to
	 * null */
	ZVAL_NULL(errmsg);

	return SUCCESS;
}

/* {{{ MongoCursor->__construct
 */
PHP_METHOD(MongoCursor, __construct) {
  zval *zlink = 0, *zns = 0, *zquery = 0, *zfields = 0, *empty, *timeout;
  zval **data;
  mongo_cursor *cursor;
  mongo_link *link;
	char *error_message = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz|zz", &zlink,
                            mongo_ce_Mongo, &zns, &zquery, &zfields) == FAILURE) {
    return;
  }
  if ((zquery && IS_SCALAR_P(zquery)) || (zfields && IS_SCALAR_P(zfields))) {
    zend_error(E_WARNING, "MongoCursor::__construct() expects parameters 3 and 4 to be arrays or objects");
    return;
  }

  // if query or fields weren't passed, make them default to an empty array
  MAKE_STD_ZVAL(empty);
  object_init(empty);

  // these are both initialized to the same zval, but that's okay because
  // there's no way to change them without creating a new cursor
  if (!zquery || (Z_TYPE_P(zquery) == IS_ARRAY && zend_hash_num_elements(HASH_P(zquery)) == 0)) {
    zquery = empty;
  }
  if (!zfields) {
    zfields = empty;
  }

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  // db connection
  cursor->resource = zlink;
  zval_add_ref(&zlink);

	/* db connection resource */
	PHP_MONGO_GET_LINK(zlink);

	/* TODO: We have to assume to use a read connection here, but it should
	 * really be refactored so that we can create a cursor with the correct
	 * read/write setup already, instead of having to force a new mode later
	 * (like we do for commands right now through
	 * php_mongo_connection_force_primary).  See also MongoDB::command and
	 * append_getlasterror, where this has to be done too. */
	cursor->connection = mongo_get_read_write_connection(link->manager, link->servers, MONGO_CON_FLAG_READ, (char**) &error_message);

	if (!cursor->connection && error_message) {
		zend_throw_exception(mongo_ce_ConnectionException, error_message, 71 TSRMLS_CC);
		return;
	}

  // change ['x', 'y', 'z'] into {'x' : 1, 'y' : 1, 'z' : 1}
  if (Z_TYPE_P(zfields) == IS_ARRAY) {
    HashPosition pointer;
    zval *fields;

    MAKE_STD_ZVAL(fields);
    array_init(fields);

    // fields to return
    for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(zfields), &pointer);
        zend_hash_get_current_data_ex(Z_ARRVAL_P(zfields), (void**) &data, &pointer) == SUCCESS;
        zend_hash_move_forward_ex(Z_ARRVAL_P(zfields), &pointer)) {
      int key_type, key_len;
      ulong index;
      char *key;

      key_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(zfields), &key, (uint*)&key_len, &index, NO_DUP, &pointer);

      if (key_type == HASH_KEY_IS_LONG) {
        if (Z_TYPE_PP(data) == IS_STRING) {
          add_assoc_long(fields, Z_STRVAL_PP(data), 1);
        }
        else {
          zval_ptr_dtor(&empty);
          zval_ptr_dtor(&fields);
          zend_throw_exception(mongo_ce_Exception, "field names must be strings", 0 TSRMLS_CC);
          return;
        }
      }
      else {
        add_assoc_zval(fields, key, *data);
        zval_add_ref(data);
      }
    }
    cursor->fields = fields;
  }
  // if it's already an object, we don't have to worry
  else {
    cursor->fields = zfields;
    zval_add_ref(&zfields);
  }

  // ns
  convert_to_string(zns);
  cursor->ns = estrdup(Z_STRVAL_P(zns));

  // query
  cursor->query = zquery;
  zval_add_ref(&zquery);

  // reset iteration pointer, just in case
  MONGO_METHOD(MongoCursor, reset, return_value, getThis());

  cursor->at = 0;
  cursor->num = 0;
  cursor->special = 0;
  cursor->persist = 0;

  timeout = zend_read_static_property(mongo_ce_Cursor, "timeout", strlen("timeout"), NOISY TSRMLS_CC);
  cursor->timeout = Z_LVAL_P(timeout);

	/* Sets the wire protocol flag to allow reading from a secondary */
	cursor->opts = cursor->connection->connection_type == MONGO_NODE_PRIMARY ? (1 << 2) : 0;

  // get rid of extra ref
  zval_ptr_dtor(&empty);
}
/* }}} */


static void make_special(mongo_cursor *cursor) {
  zval *temp;
  if (cursor->special) {
    return;
  }

  cursor->special = 1;

  temp = cursor->query;
  MAKE_STD_ZVAL(cursor->query);
  array_init(cursor->query);
  add_assoc_zval(cursor->query, "$query", temp);
}

/* {{{ MongoCursor::hasNext
 */
PHP_METHOD(MongoCursor, hasNext) {
  buffer buf;
  int size;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	char *error_message = NULL;
	zval *temp;

	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  if (!cursor->started_iterating) {
    MONGO_METHOD(MongoCursor, doQuery, return_value, getThis());
    cursor->started_iterating = 1;
  }

	if ((cursor->limit > 0 && cursor->at >= cursor->limit) || cursor->num == 0) {
		if (cursor->cursor_id != 0) {
			mongo_cursor_free_le(cursor, MONGO_CURSOR TSRMLS_CC);
		}
		RETURN_FALSE;
	}
  if (cursor->at < cursor->num) {
    RETURN_TRUE;
  }
  else if (cursor->cursor_id == 0) {
    RETURN_FALSE;
  }
  // if we have a cursor_id, we should have a server
	else if (cursor->connection == 0) {
    mongo_cursor_throw(0, 18 TSRMLS_CC, "trying to get more, but cannot find server");
    return;
  }

  // we have to go and check with the db
  size = 34+strlen(cursor->ns);
  CREATE_BUF(buf, size);
  if (FAILURE == php_mongo_write_get_more(&buf, cursor TSRMLS_CC)) {
    efree(buf.start);
    return;
  }

	if (mongo_io_send(cursor->connection->socket, buf.start, buf.pos - buf.start, (char**) &error_message) == -1) {
		efree(buf.start);

		mongo_cursor_throw(cursor->connection, 1 TSRMLS_CC, error_message);
		free(error_message);
		mongo_util_cursor_failed(cursor TSRMLS_CC);
		return;
	}

	efree(buf.start);

	MAKE_STD_ZVAL(temp);
	ZVAL_NULL(temp);

  if (php_mongo_get_reply(cursor, temp TSRMLS_CC) != SUCCESS) {
	free(error_message);
    mongo_util_cursor_failed(cursor TSRMLS_CC);
    return;
  }

  zval_ptr_dtor(&temp);

  if (cursor->cursor_id == 0) {
    mongo_cursor_free_le(cursor, MONGO_CURSOR TSRMLS_CC);
  }
  // if cursor_id != 0, server should stay the same

  if (cursor->flag & 1) {
		mongo_cursor_throw(cursor->connection, 2 TSRMLS_CC, "cursor not found");
    return;
  }

  // sometimes we'll have a cursor_id but there won't be any more results
  if (cursor->at >= cursor->num) {
    RETURN_FALSE;
  }
  // but sometimes there will be
  else {
    RETURN_TRUE;
  }
}
/* }}} */

/* {{{ MongoCursor::getNext
 */
PHP_METHOD(MongoCursor, getNext) {
  MONGO_METHOD(MongoCursor, next, return_value, getThis());
  // will be null unless there was an error
  if (EG(exception) ||
      (Z_TYPE_P(return_value) == IS_BOOL && Z_BVAL_P(return_value) == 0)) {
		ZVAL_NULL(return_value);
    return;
  }
  MONGO_METHOD(MongoCursor, current, return_value, getThis());
}
/* }}} */

/* {{{ MongoCursor::limit
 */
PHP_METHOD(MongoCursor, limit) {
  long l;
  preiteration_setup;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &l) == FAILURE) {
    return;
  }

  cursor->limit = l;
  RETVAL_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::batchSize
 */
PHP_METHOD(MongoCursor, batchSize) {
  long l;
  preiteration_setup;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &l) == FAILURE) {
    return;
  }

  cursor->batch_size = l;
  RETVAL_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::skip
 */
PHP_METHOD(MongoCursor, skip) {
  long l;
  preiteration_setup;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &l) == FAILURE) {
    return;
  }

  cursor->skip = l;
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::fields
 */
PHP_METHOD(MongoCursor, fields) {
  zval *z;
  preiteration_setup;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(z)) {
    zend_error(E_WARNING, "MongoCursor::fields() expects parameter 1 to be an array or object");
    return;
  }

  zval_ptr_dtor(&cursor->fields);
  cursor->fields = z;
  zval_add_ref(&z);

  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::dead
 */
PHP_METHOD(MongoCursor, dead) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  RETURN_BOOL(cursor->started_iterating && cursor->cursor_id == 0);
}
/* }}} */

/* {{{ Cursor flags
 * Sets or unsets the flag <flag>. With mode = -1, the arguments are parsed.
 * Otherwise the mode should contain 0 for unsetting and 1 for setting the flag.
 */
static inline void set_cursor_flag(INTERNAL_FUNCTION_PARAMETERS, int flag, int mode)
{
	zend_bool z = 1;

	preiteration_setup;
	if (mode == -1) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &z) == FAILURE) {
			return;
		}
	} else {
		z = mode;
	}

	if (z) {
		cursor->opts |= flag;
	} else {
		cursor->opts &= ~flag;
	}

	RETURN_ZVAL(getThis(), 1, 0);
}

/* {{{ MongoCursor::setFlag(int bit [, bool set])
 */
PHP_METHOD(MongoCursor, setFlag)
{
	long      bit;
	zend_bool set = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|b", &bit, &set) == FAILURE) {
		return;
	}
	/* Prevent bit 3 (CURSOR_FLAG_OPLOG_REPLAY) and bit 6 (CURSOR_FLAG_EXHAUST) from
	 * being set. The first because it's an internal flag, and the second because
	 * the driver can't handle this at the moment. */
	if (bit == 3 || bit == 6) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The CURSOR_FLAG_OPLOG_REPLAY(3) and CURSOR_FLAG_EXHAUST(6) flags are not supported.");
		return;
	}
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1 << bit, set);
}
/* }}} */

/* {{{ MongoCursor::tailable(bool flag)
 */
PHP_METHOD(MongoCursor, tailable)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_TAILABLE, -1);
}
/* }}} */

/* {{{ MongoCursor::slaveOkay(bool flag)
 */
PHP_METHOD(MongoCursor, slaveOkay)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_SLAVE_OKAY, -1);
}
/* }}} */


/* {{{ MongoCursor::immortal(bool flag)
 */
PHP_METHOD(MongoCursor, immortal)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_NO_CURSOR_TO, -1);
}
/* }}} */

/* {{{ MongoCursor::awaitData(bool flag)
 */
PHP_METHOD(MongoCursor, awaitData)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_AWAIT_DATA, -1);
}
/* }}} */

/* {{{ MongoCursor::partial(bool flag)
 */
PHP_METHOD(MongoCursor, partial)
{
	set_cursor_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, CURSOR_FLAG_PARTIAL, -1);
}
/* }}} */
/* }}} */


/* {{{ MongoCursor::timeout
 */
PHP_METHOD(MongoCursor, timeout) {
  long timeout;
  mongo_cursor *cursor;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &timeout) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_CURSOR(getThis());

  cursor->timeout = timeout;

  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::addOption
 */
PHP_METHOD(MongoCursor, addOption) {
  char *key;
  int key_len;
  zval *query, *value;
  mongo_cursor *cursor;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &key, &key_len, &value) == FAILURE) {
    return;
  }

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  if (cursor->started_iterating) {
    mongo_cursor_throw(cursor->connection, 0 TSRMLS_CC, "cannot modify cursor after beginning iteration");
    return;
  }

  make_special(cursor);
  query = cursor->query;
  add_assoc_zval(query, key, value);
  zval_add_ref(&value);

  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::snapshot
 */
PHP_METHOD(MongoCursor, snapshot) {
  zval *snapshot, *yes;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  MAKE_STD_ZVAL(snapshot);
  ZVAL_STRING(snapshot, "$snapshot", 1);
  MAKE_STD_ZVAL(yes);
  ZVAL_TRUE(yes);

  MONGO_METHOD2(MongoCursor, addOption, return_value, getThis(), snapshot, yes);

  zval_ptr_dtor(&snapshot);
  zval_ptr_dtor(&yes);
}
/* }}} */


/* {{{ MongoCursor->sort(array fields)
 */
PHP_METHOD(MongoCursor, sort) {
  zval *orderby, *fields;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &fields) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(fields)) {
    zend_error(E_WARNING, "MongoCursor::sort() expects parameter 1 to be an array");
    return;
  }

  MAKE_STD_ZVAL(orderby);
  ZVAL_STRING(orderby, "$orderby", 1);

  MONGO_METHOD2(MongoCursor, addOption, return_value, getThis(), orderby, fields);

  zval_ptr_dtor(&orderby);
}
/* }}} */

/* {{{ MongoCursor->hint
 */
PHP_METHOD(MongoCursor, hint) {
  zval *hint, *fields;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &fields) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(fields)) {
    zend_error(E_WARNING, "MongoCursor::hint() expects parameter 1 to be an array or object");
    return;
  }


  MAKE_STD_ZVAL(hint);
  ZVAL_STRING(hint, "$hint", 1);

  MONGO_METHOD2(MongoCursor, addOption, return_value, getThis(), hint, fields);

  zval_ptr_dtor(&hint);
}
/* }}} */

/* {{{ MongoCursor->getCursorInfo: Return information about the current query (by @crodas)
 */
PHP_METHOD(MongoCursor, info)
{
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);
  array_init(return_value);

  add_assoc_string(return_value, "ns", cursor->ns, 1);
  add_assoc_long(return_value, "limit", cursor->limit);
  add_assoc_long(return_value, "batchSize", cursor->batch_size);
  add_assoc_long(return_value, "skip", cursor->skip);
  add_assoc_long(return_value, "flags", cursor->opts);
  if (cursor->query) {
    add_assoc_zval(return_value, "query", cursor->query);
    zval_add_ref(&cursor->query);
  } else {
    add_assoc_null(return_value, "query");
  }
  if (cursor->fields) {
    add_assoc_zval(return_value, "fields", cursor->fields);
    zval_add_ref(&cursor->fields);
  } else {
    add_assoc_null(return_value, "fields");
  }

  add_assoc_bool(return_value, "started_iterating", cursor->started_iterating);
  if (cursor->started_iterating) {
    add_assoc_long(return_value, "id", (long)cursor->cursor_id);
    add_assoc_long(return_value, "at", cursor->at);
    add_assoc_long(return_value, "numReturned", cursor->num);
		/* TODO: Use real host instead of hash */
		add_assoc_string(return_value, "server", cursor->connection->hash, 1);
  }
}
/* }}} */

/* {{{ MongoCursor->explain
 */
PHP_METHOD(MongoCursor, explain) {
  int temp_limit;
  zval *explain, *yes, *temp = 0;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  MONGO_METHOD(MongoCursor, reset, return_value, getThis());

  // make explain use a hard limit
  temp_limit = cursor->limit;
  if (cursor->limit > 0) {
    cursor->limit *= -1;
  }

  MAKE_STD_ZVAL(explain);
  ZVAL_STRING(explain, "$explain", 1);
  MAKE_STD_ZVAL(yes);
  ZVAL_TRUE(yes);

  MONGO_METHOD2(MongoCursor, addOption, return_value, getThis(), explain, yes);

  zval_ptr_dtor(&explain);
  zval_ptr_dtor(&yes);

  MONGO_METHOD(MongoCursor, getNext, return_value, getThis());

  // reset cursor to original state
  cursor->limit = temp_limit;
  zend_hash_del(HASH_P(cursor->query), "$explain", strlen("$explain")+1);

  MAKE_STD_ZVAL(temp);
  ZVAL_NULL(temp);
  MONGO_METHOD(MongoCursor, reset, temp, getThis());
  zval_ptr_dtor(&temp);
}
/* }}} */


/* {{{ MongoCursor->doQuery
 */
PHP_METHOD(MongoCursor, doQuery) {
  mongo_cursor *cursor;

  PHP_MONGO_GET_CURSOR(getThis());

  do {
    MONGO_METHOD(MongoCursor, reset, return_value, getThis());
    if (mongo_cursor__do_query(getThis(), return_value TSRMLS_CC) == SUCCESS ||
        EG(exception)) {
      return;
    }
  } while (mongo_cursor__should_retry(cursor));

  if (strcmp(".$cmd", cursor->ns+(strlen(cursor->ns)-5)) == 0) {
		mongo_cursor_throw(cursor->connection, 19 TSRMLS_CC, "couldn't send command");
    return;
  }

	mongo_cursor_throw(cursor->connection, 19 TSRMLS_CC, "max number of retries exhausted, couldn't send query");
}

int mongo_cursor__do_query(zval *this_ptr, zval *return_value TSRMLS_DC) {
  mongo_cursor *cursor;
  buffer buf;
  zval *errmsg;
	char *error_message;

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  if (!cursor) {
    zend_throw_exception(mongo_ce_Exception,
                         "The MongoCursor object has not been correctly initialized by its constructor",
                         0 TSRMLS_CC);
    return FAILURE;
  }

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (php_mongo_write_query(&buf, cursor TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return FAILURE;
  }

#if 0
  // If slave_okay is set, read from a slave.
  if ((cursor->link->rs && cursor->opts & CURSOR_FLAG_SLAVE_OKAY &&
       (cursor->connection = mongo_util_link_get_slave_socket(cursor->link, errmsg TSRMLS_CC)) == 0)) {
    // ignore errors and reset errmsg
    zval_ptr_dtor(&errmsg);
    MAKE_STD_ZVAL(errmsg);
    ZVAL_NULL(errmsg);
  }

  // if getting the slave didn't work (or we're not using a rs), just get master socket
  if (cursor->connection == 0 &&
      (cursor->connection = mongo_util_link_get_socket(cursor->link, errmsg TSRMLS_CC)) == 0) {
    efree(buf.start);

    // if we couldn't connect to the master or the slave
    if (cursor->opts & CURSOR_FLAG_SLAVE_OKAY) {
      mongo_cursor_throw(0, 14 TSRMLS_CC, "couldn't get a connection to any server");
    }
    else {
      mongo_cursor_throw(0, 14 TSRMLS_CC, Z_STRVAL_P(errmsg));
    }

    zval_ptr_dtor(&errmsg);
    return FAILURE;
  }
#endif
	if (mongo_io_send(cursor->connection->socket, buf.start, buf.pos - buf.start, (char **) &error_message) == -1) {
		if (error_message) {
			mongo_cursor_throw(cursor->connection, 14 TSRMLS_CC, "couldn't send query: %s", error_message);
			free(error_message);
		} else {
			mongo_cursor_throw(cursor->connection, 14 TSRMLS_CC, "couldn't send query");
		}
		efree(buf.start);
		
		return mongo_util_cursor_failed(cursor TSRMLS_CC);
	}

	efree(buf.start);

	MAKE_STD_ZVAL(errmsg);
	ZVAL_NULL(errmsg);
  if (php_mongo_get_reply(cursor, errmsg TSRMLS_CC) == FAILURE) {
    zval_ptr_dtor(&errmsg);
    return mongo_util_cursor_failed(cursor TSRMLS_CC);
  }

  zval_ptr_dtor(&errmsg);

  /* we've got something to kill, make a note */
  if (cursor->cursor_id != 0) {
    php_mongo_create_le(cursor, "cursor_list" TSRMLS_CC);
  }

  return SUCCESS;
}
/* }}} */
#if 0
int mongo_util_cursor_failed(mongo_cursor *cursor TSRMLS_DC) {
  mongo_server *old = cursor->connection;

  // kill cursor so that the server stops and the new connection doesn't try
  // to kill something it doesn't own
  mongo_util_cursor_reset(cursor TSRMLS_CC);

  // reset sets cursor->connection to 0, so we use "old" here
  mongo_util_link_failed(cursor->link, old TSRMLS_CC);

  return FAILURE;
}
#endif

// ITERATOR FUNCTIONS

/* {{{ MongoCursor->current
 */
PHP_METHOD(MongoCursor, current) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  if (cursor->current) {
    RETURN_ZVAL(cursor->current, 1, 0);
  }
  else {
    RETURN_NULL();
  }
}
/* }}} */

/* {{{ MongoCursor->key
 */
PHP_METHOD(MongoCursor, key) {
  zval **id;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  if (!cursor->current) {
    RETURN_NULL();
  }
  if (cursor->current &&
      Z_TYPE_P(cursor->current) == IS_ARRAY &&
      zend_hash_find(HASH_P(cursor->current), "_id", 4, (void**)&id) == SUCCESS) {

    if (Z_TYPE_PP(id) == IS_OBJECT) {
#if ZEND_MODULE_API_NO >= 20060613
      zend_std_cast_object_tostring(*id, return_value, IS_STRING TSRMLS_CC);
#else
      zend_std_cast_object_tostring(*id, return_value, IS_STRING, 0 TSRMLS_CC);
#endif /* ZEND_MODULE_API_NO >= 20060613 */
    }
    else {
      RETVAL_ZVAL(*id, 1, 0);
      convert_to_string(return_value);
    }
  }
  else {
    RETURN_LONG(cursor->at - 1);
  }
}
/* }}} */

int mongo_cursor__should_retry(mongo_cursor *cursor) {
  int microseconds = 50000, slots = 0, wait_us = 0;

  // never retry commands
  if (cursor->retry >= 5 ||
      strcmp(".$cmd", cursor->ns+(strlen(cursor->ns)-5)) == 0) {
    return 0;
  }

  slots = (int)pow(2.0, cursor->retry++);
  wait_us = (rand() % slots) * microseconds;

#ifdef WIN32
  // windows sleep takes milliseconds
  Sleep(wait_us/1000);
#else
  {
    // usleep is deprecated
    struct timespec wait;

    wait.tv_sec = wait_us / 1000000;
    wait.tv_nsec = (wait_us % 1000000) * 1000;

    nanosleep(&wait, 0);
  }
#endif

  return 1;
}

/* {{{ MongoCursor->next
 */
PHP_METHOD(MongoCursor, next) {
  zval has_next;
  mongo_cursor *cursor;

  PHP_MONGO_GET_CURSOR(getThis());

  if (!cursor->started_iterating) {
    MONGO_METHOD(MongoCursor, doQuery, return_value, getThis());
    if (EG(exception)) {
      return;
    }
    cursor->started_iterating = 1;
  }

  // destroy old current
  if (cursor->current) {
    zval_ptr_dtor(&cursor->current);
    cursor->current = 0;
  }

  // check for results
  MONGO_METHOD(MongoCursor, hasNext, &has_next, getThis());
  if (EG(exception)) {
    return;
  }
  if (!Z_BVAL(has_next)) {
    // we're out of results
    RETURN_NULL();
  }

  // we got more results
  if (cursor->at < cursor->num) {
    zval **err = 0;

    MAKE_STD_ZVAL(cursor->current);
    array_init(cursor->current);
    cursor->buf.pos = bson_to_zval((char*)cursor->buf.pos, Z_ARRVAL_P(cursor->current) TSRMLS_CC);

    if (EG(exception)) {
      zval_ptr_dtor(&cursor->current);
      cursor->current = 0;
      return;
    }

    // increment cursor position
    cursor->at++;

    // check for $err
    if (zend_hash_find(Z_ARRVAL_P(cursor->current), "$err", strlen("$err")+1, (void**)&err) == SUCCESS ||
        // getLastError can return an error here
        (zend_hash_find(Z_ARRVAL_P(cursor->current), "err", strlen("err")+1, (void**)&err) == SUCCESS &&
         Z_TYPE_PP(err) == IS_STRING)) {
      zval **code_z, *exception;
      // default error code
      int code = 4;

      if (zend_hash_find(Z_ARRVAL_P(cursor->current), "code", strlen("code")+1, (void**)&code_z) == SUCCESS) {
        // check for not master
        if (Z_TYPE_PP(code_z) == IS_LONG) {
          code = Z_LVAL_PP(code_z);
        }
        else if (Z_TYPE_PP(code_z) == IS_DOUBLE) {
          code = (int)Z_DVAL_PP(code_z);
        }
        // else code == 4
#if 0
        // this shouldn't be necessary after 1.7.* is standard, it forces
        // failover in case the master steps down.
        // not master: 10107
        // not master and slaveok=false (more recent): 13435
        // not master or secondary: 13436
        if (cursor->link->rs && (code == 10107 || code == 13435 || code == 13436 || code == 10058)) {
          mongo_util_link_master_failed(cursor->link TSRMLS_CC);
        }
#endif
      }

      exception = mongo_cursor_throw(cursor->connection, code TSRMLS_CC, Z_STRVAL_PP(err));
      zend_update_property(mongo_ce_CursorException, exception, "doc", strlen("doc"), cursor->current TSRMLS_CC);
      zval_ptr_dtor(&cursor->current);
      cursor->current = 0;
      RETURN_FALSE;
    }
  }

  RETURN_NULL();
}
/* }}} */

/* {{{ MongoCursor->rewind
 */
PHP_METHOD(MongoCursor, rewind) {
  MONGO_METHOD(MongoCursor, reset, return_value, getThis());
  MONGO_METHOD(MongoCursor, next, return_value, getThis());
}
/* }}} */

/* {{{ MongoCursor->valid
 */
PHP_METHOD(MongoCursor, valid) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  RETURN_BOOL(cursor->current);
}
/* }}} */

/* {{{ MongoCursor->reset
 */
PHP_METHOD(MongoCursor, reset) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED(cursor->connection, MongoCursor);

  mongo_util_cursor_reset(cursor TSRMLS_CC);
}

void mongo_util_cursor_reset(mongo_cursor *cursor TSRMLS_DC) {
  cursor->buf.pos = cursor->buf.start;

  if (cursor->current) {
    zval_ptr_dtor(&cursor->current);
  }

  if (cursor->cursor_id != 0) {
    mongo_cursor_free_le(cursor, MONGO_CURSOR TSRMLS_CC);
    cursor->cursor_id = 0;
  }

  cursor->started_iterating = 0;
  cursor->current = 0;
  cursor->at = 0;
  cursor->num = 0;
}
/* }}} */

PHP_METHOD(MongoCursor, count) {
  zval *db_z, *coll, *query;
  mongo_cursor *cursor;
  mongo_collection *c;
  mongo_db *db;
  zend_bool all = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &all) == FAILURE) {
    return;
  }

  PHP_MONGO_GET_CURSOR(getThis());

  // fake a MongoDB object
  MAKE_STD_ZVAL(db_z);
  object_init_ex(db_z, mongo_ce_DB);
  db = (mongo_db*)zend_object_store_get_object(db_z TSRMLS_CC);
  db->link = cursor->resource;
  MAKE_STD_ZVAL(db->name);
  ZVAL_STRING(db->name, estrndup(cursor->ns, strchr(cursor->ns, '.') - cursor->ns), 0);

  // fake a MongoCollection object
  MAKE_STD_ZVAL(coll);
  object_init_ex(coll, mongo_ce_Collection);
  c = (mongo_collection*)zend_object_store_get_object(coll TSRMLS_CC);
  MAKE_STD_ZVAL(c->ns);
  ZVAL_STRING(c->ns, estrdup(cursor->ns), 0);
  MAKE_STD_ZVAL(c->name);
  ZVAL_STRING(c->name, estrdup(cursor->ns + (strchr(cursor->ns, '.') - cursor->ns) + 1), 0);
  c->parent = db_z;

  if (cursor->query) {
    zval **inner_query = 0;

    if (!cursor->special) {
      query = cursor->query;
      zval_add_ref(&query);
    }
    else if (zend_hash_find(HASH_P(cursor->query), "$query", strlen("$query")+1, (void**)&inner_query) == SUCCESS) {
      query = *inner_query;
      zval_add_ref(&query);
    }
  }
  else {
    MAKE_STD_ZVAL(query);
    array_init(query);
  }

  if (all) {
    zval *limit_z, *skip_z;

    MAKE_STD_ZVAL(limit_z);
    MAKE_STD_ZVAL(skip_z);

    ZVAL_LONG(limit_z, cursor->limit);
    ZVAL_LONG(skip_z, cursor->skip);

    MONGO_METHOD3(MongoCollection, count, return_value, coll, query, limit_z, skip_z);

    zval_ptr_dtor(&limit_z);
    zval_ptr_dtor(&skip_z);
  }
  else {
    MONGO_METHOD1(MongoCollection, count, return_value, coll, query);
  }

  zval_ptr_dtor(&query);

  c->parent = 0;
  zend_objects_store_del_ref(coll TSRMLS_CC);
  zval_ptr_dtor(&coll);

  db->link = 0;
  zend_objects_store_del_ref(db_z TSRMLS_CC);
  zval_ptr_dtor(&db_z);
}
 
ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(0, connection, Mongo, 0)
	ZEND_ARG_INFO(0, database_and_collection_name)
	ZEND_ARG_ARRAY_INFO(0, query, 0)
	ZEND_ARG_INFO(0, array_of_fields_OR_object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_no_parameters, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_batchsize, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_skip, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, number)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fields, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, fields, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_add_option, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, fields, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hint, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_ARRAY_INFO(0, keyPattern, 0)
ZEND_END_ARG_INFO()

/* {{{ Cursor flags */
ZEND_BEGIN_ARG_INFO_EX(arginfo_set_flag, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, bit)
	ZEND_ARG_INFO(0, set)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tailable, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, tail)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_slave_okay, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, okay)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_immortal, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, liveForever)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_await_data, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, wait)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_partial, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, okay)
ZEND_END_ARG_INFO()
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo_timeout, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, milliseconds)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_count, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, foundOnly)
ZEND_END_ARG_INFO()


static zend_function_entry MongoCursor_methods[] = {
  PHP_ME(MongoCursor, __construct, arginfo___construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hasNext, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, getNext, arginfo_no_parameters, ZEND_ACC_PUBLIC)

  /* options */
  PHP_ME(MongoCursor, limit, arginfo_limit, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, batchSize, arginfo_batchsize, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, skip, arginfo_skip, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, fields, arginfo_fields, ZEND_ACC_PUBLIC)

  /* meta options */
  PHP_ME(MongoCursor, addOption, arginfo_add_option, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, snapshot, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, sort, arginfo_sort, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hint, arginfo_hint, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, explain, arginfo_no_parameters, ZEND_ACC_PUBLIC)

  /* flags */
  PHP_ME(MongoCursor, setFlag, arginfo_set_flag, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, slaveOkay, arginfo_slave_okay, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, tailable, arginfo_tailable, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, immortal, arginfo_immortal, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, awaitData, arginfo_await_data, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, partial, arginfo_partial, ZEND_ACC_PUBLIC)

  /* query */
  PHP_ME(MongoCursor, timeout, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, doQuery, arginfo_no_parameters, ZEND_ACC_PROTECTED|ZEND_ACC_DEPRECATED)
  PHP_ME(MongoCursor, info, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, dead, arginfo_no_parameters, ZEND_ACC_PUBLIC)

  /* iterator funcs */
  PHP_ME(MongoCursor, current, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, key, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, next, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, rewind, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, valid, arginfo_no_parameters, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, reset, arginfo_no_parameters, ZEND_ACC_PUBLIC)

  /* stand-alones */
  PHP_ME(MongoCursor, count, arginfo_count, ZEND_ACC_PUBLIC)

  {NULL, NULL, NULL}
};


PHP_METHOD(MongoCursorException, getHost) {
  zval *h;

  h = zend_read_property(mongo_ce_CursorException, getThis(), "host", strlen("host"), NOISY TSRMLS_CC);

  RETURN_ZVAL(h, 1, 0);
}

static zend_function_entry cursor_exception_methods[] = {
  PHP_ME(MongoCursorException, getHost, NULL, ZEND_ACC_PUBLIC)

  {NULL, NULL, NULL}
};

void mongo_init_CursorExceptions(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoCursorException", cursor_exception_methods);
  mongo_ce_CursorException = zend_register_internal_class_ex(&ce, mongo_ce_Exception, NULL TSRMLS_CC);

  zend_declare_property_null(mongo_ce_CursorException, "host", strlen("host"), ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_long(mongo_ce_CursorException, "fd", strlen("fd"), 0, ZEND_ACC_PRIVATE TSRMLS_CC);
}



zval* mongo_cursor_throw(mongo_connection *connection, int code TSRMLS_DC, char *format, ...)
{
  zval *e;
	char *message;
	va_list arg;
	zend_class_entry *exception_ce;

  if (EG(exception)) {
    return EG(exception);
  }

	/* Based on the status, we pick a different exception class. Right now, we
	 * choose mongo_ce_CursorException for everything but status 80, which is a
	 * cursor timeout instead. */
	if (code == 80) {
		exception_ce = mongo_ce_CursorTOException;
	} else {
		exception_ce = mongo_ce_CursorException;
	}

	va_start(arg, format);
	message = malloc(1024);
	vsnprintf(message, 1024, format, arg);
	va_end(arg);
	e = zend_throw_exception_ex(exception_ce, code TSRMLS_CC, "%s", message);
	free(message);

	if (connection && code != 80) {
		/* TODO: Use host instead of the hash */
		zend_update_property_string(exception_ce, e, "host", strlen("host"), connection->hash TSRMLS_CC);
		zend_update_property_long(exception_ce, e, "fd", strlen("fd"), connection->socket TSRMLS_CC);
	}

	return e;
}


void mongo_cursor_free_le(void *val, int type TSRMLS_DC) {
  zend_rsrc_list_entry *le;

  LOCK(cursor);

  /*
   * This should work if le->ptr is null or non-null
   */
  if (zend_hash_find(&EG(persistent_list), "cursor_list", strlen("cursor_list") + 1, (void**)&le) == SUCCESS) {
    cursor_node *current;

    current = le->ptr;

    while (current) {
      cursor_node *next = current->next;

      if (type == MONGO_SERVER) {
        mongo_server *server = (mongo_server*)val;
        if (server != 0 && current->socket == server->socket) {
          if (!server->connected) {
            php_mongo_free_cursor_node(current, le);
          }
          else {
            kill_cursor(current, le TSRMLS_CC);
          }
          // keep going, free all cursor for this connection
        }
      }
      else if (type == MONGO_CURSOR) {
        mongo_cursor *cursor = (mongo_cursor*)val;
        if (current->cursor_id == cursor->cursor_id &&
            cursor->connection != 0 &&
            current->socket == cursor->connection->socket) {

          // If the cursor_id is 0, the db is out of results anyway
          // If the connection is not connected, just return
          if (current->cursor_id == 0/* || !cursor->connection->connected*/) {
            php_mongo_free_cursor_node(current, le);
          }
          else {
            kill_cursor(current, le TSRMLS_CC);

            /*
             * if the connection is closed before the cursor is destroyed, the cursor
             * might try to fetch more results with disasterous consequences.  Thus, the
             * cursor_id is set to 0, so no more results will be fetched.
             *
             * this might not be the most elegant solution, since you could fetch 100
             * results, get the first one, close the connection, get 99 more, and suddenly
             * not be able to get any more.  Not sure if there's a better one, though. I
             * guess the user can call dead() on the cursor.
             */
            cursor->cursor_id = 0;
          }
          // only one cursor to be freed
          break;
        }
      }

      current = next;
    }
  }

  UNLOCK(cursor);
}


int php_mongo_create_le(mongo_cursor *cursor, char *name TSRMLS_DC) {
  zend_rsrc_list_entry *le;
  cursor_node *new_node;

  LOCK(cursor);

  new_node = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
  new_node->cursor_id = cursor->cursor_id;
	if (cursor->connection) {
		new_node->socket = cursor->connection->socket;
  }
  else {
    new_node->socket = 0;
  }
  new_node->next = new_node->prev = 0;

  /*
   * 3 options:
   *   - le doesn't exist
   *   - le exists and is null
   *   - le exists and has elements
   * In case 1 & 2, we want to create a new le ptr, otherwise we want to append
   * to the existing ptr.
   */
  if (zend_hash_find(&EG(persistent_list), name, strlen(name)+1, (void**)&le) == SUCCESS) {
    cursor_node *current = le->ptr;
    cursor_node *prev = 0;

    if (current == 0) {
      le->ptr = new_node;
      UNLOCK(cursor);
      return 0;
    }

    do {
      /*
       * if we find the current cursor in the cursor list, we don't need another
       * dtor for it so unlock the mutex & return.
       */
      if (current->cursor_id == cursor->cursor_id &&
          current->socket == cursor->connection->socket) {
        pefree(new_node, 1);
        UNLOCK(cursor);
        return 0;
      }

      prev = current;
      current = current->next;
    }
    while (current);

    /*
     * we didn't find the cursor.  add it to the list. (prev is pointing to the
     * tail of the list, current is pointing to null.
     */
    prev->next = new_node;
    new_node->prev = prev;
  }
  else {
    zend_rsrc_list_entry new_le;
    new_le.ptr = new_node;
    new_le.type = le_cursor_list;
    new_le.refcount = 1;
    zend_hash_add(&EG(persistent_list), name, strlen(name)+1, &new_le, sizeof(zend_rsrc_list_entry), NULL);
  }

  UNLOCK(cursor);
  return 0;
}

static int cursor_list_pfree_helper(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  LOCK(cursor);

  {
    cursor_node *node = (cursor_node*)rsrc->ptr;

    if (!node) {
      UNLOCK(cursor);
      return 0;
    }

    while (node->next) {
      cursor_node *temp = node;
      node = node->next;
      pefree(temp, 1);
    }
    pefree(node, 1);
  }

  UNLOCK(cursor);
  return 0;
}

void php_mongo_cursor_list_pfree(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  cursor_list_pfree_helper(rsrc TSRMLS_CC);
}

void php_mongo_free_cursor_node(cursor_node *node, zend_rsrc_list_entry *le) {

  /*
   * [node1][<->][NODE2][<->][node3]
   *   [node1][->][node3]
   *   [node1][<->][node3]
   *
   * [node1][<->][NODE2]
   *   [node1]
   */
  if (node->prev) {
    node->prev->next = node->next;
    if (node->next) {
      node->next->prev = node->prev;
    }
  }
  /*
   * [NODE2][<->][node3]
   *   le->ptr = node3
   *   [node3]
   *
   * [NODE2]
   *   le->ptr = 0
   */
  else {
    le->ptr = node->next;
    if (node->next) {
      node->next->prev = 0;
    }
  }

  pefree(node, 1);
}

// tell db to destroy its cursor
static void kill_cursor(cursor_node *node, zend_rsrc_list_entry *le TSRMLS_DC) {
  char quickbuf[128];
  buffer buf;
  zval temp;

  /*
   * If the cursor_id is 0, the db is out of results anyway.
   */
  if (node->cursor_id == 0) {
    php_mongo_free_cursor_node(node, le);
    return;
  }

  buf.pos = quickbuf;
  buf.start = buf.pos;
  buf.end = buf.start + 128;

  php_mongo_write_kill_cursors(&buf, node->cursor_id TSRMLS_CC);

  Z_TYPE(temp) = IS_NULL;
  _mongo_say(node->socket, &buf, &temp TSRMLS_CC);
  if (Z_TYPE(temp) == IS_STRING) {
    efree(Z_STRVAL(temp));
    Z_TYPE(temp) = IS_NULL;
  }

  // free this cursor/link pair
  php_mongo_free_cursor_node(node, le);
}


static zend_object_value php_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC) {
  php_mongo_obj_new(mongo_cursor);
}


void php_mongo_cursor_free(void *object TSRMLS_DC) {
  mongo_cursor *cursor = (mongo_cursor*)object;

  if (cursor) {
    if (cursor->cursor_id != 0) {
      mongo_cursor_free_le(cursor, MONGO_CURSOR TSRMLS_CC);
    }

    if (cursor->current) zval_ptr_dtor(&cursor->current);

    if (cursor->query) zval_ptr_dtor(&cursor->query);
    if (cursor->fields) zval_ptr_dtor(&cursor->fields);

    if (cursor->buf.start) efree(cursor->buf.start);
    if (cursor->ns) efree(cursor->ns);

    if (cursor->resource) zval_ptr_dtor(&cursor->resource);

    zend_object_std_dtor(&cursor->std TSRMLS_CC);

    efree(cursor);
  }
}

void mongo_init_MongoCursor(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoCursor", MongoCursor_methods);
  ce.create_object = php_mongo_cursor_new;
  mongo_ce_Cursor = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(mongo_ce_Cursor TSRMLS_CC, 1, zend_ce_iterator);

  zend_declare_property_bool(mongo_ce_Cursor, "slaveOkay", strlen("slaveOkay"), 0, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_Cursor, "timeout", strlen("timeout"), 30000L, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC TSRMLS_CC);
}

