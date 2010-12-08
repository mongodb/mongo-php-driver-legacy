//cursor.c
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

#ifdef WIN32
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#endif

#include <zend_interfaces.h>
#include <zend_exceptions.h>

#include "php_mongo.h"
#include "bson.h"
#include "db.h"
#include "cursor.h"
#include "collection.h"
#include "mongo_types.h"


// externs
extern zend_class_entry *mongo_ce_Id,
  *mongo_ce_Mongo,
  *mongo_ce_DB,
  *mongo_ce_Collection,
  *mongo_ce_Exception,
  *mongo_ce_CursorException;

extern int le_pconnection,
  le_cursor_list;

extern zend_object_handlers mongo_default_handlers;


ZEND_EXTERN_MODULE_GLOBALS(mongo);

static zend_object_value php_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC);
static void make_special(mongo_cursor *);

zend_class_entry *mongo_ce_Cursor = NULL;

/* {{{ MongoCursor->__construct
 */
PHP_METHOD(MongoCursor, __construct) {
  zval *zlink = 0, *zns = 0, *zquery = 0, *zfields = 0, *empty, *timeout;
  zval **data;
  mongo_cursor *cursor;
  mongo_link *link;

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

  // db connection resource
  PHP_MONGO_GET_LINK(zlink);
  cursor->link = link;

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

  cursor->opts = link->slave_okay ? (1 << 2) : 0;
  
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
  zval *temp;

  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  if (!cursor->started_iterating) {
    MONGO_METHOD(MongoCursor, doQuery, return_value, getThis());
    cursor->started_iterating = 1;
  }

  if ((cursor->limit > 0 && cursor->at >= cursor->limit) || 
      cursor->num == 0) {
    RETURN_FALSE;
  }
  if (cursor->at < cursor->num) {
    RETURN_TRUE;
  }
  else if (cursor->cursor_id == 0) {
    RETURN_FALSE;
  }
  // if we have a cursor_id, we should have a server
  else if (cursor->server == 0) {
    zend_throw_exception(mongo_ce_CursorException, "Trying to get more, but cannot find server", 18 TSRMLS_CC);
    return;
  }

  // we have to go and check with the db
  size = 34+strlen(cursor->ns);
  CREATE_BUF(buf, size);
  if (FAILURE == php_mongo_write_get_more(&buf, cursor TSRMLS_CC)) {
    efree(buf.start);
    return;
  }
  
  MAKE_STD_ZVAL(temp);
  ZVAL_NULL(temp);

  if(mongo_say(cursor->server->socket, &buf, temp TSRMLS_CC) == FAILURE) {
    php_mongo_disconnect_server(cursor->server);
    efree(buf.start);
    zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_P(temp), 1 TSRMLS_CC);
    zval_ptr_dtor(&temp);
    return;
  }

  efree(buf.start);

  if (php_mongo_get_reply(cursor, temp TSRMLS_CC) != SUCCESS) {
    zval_ptr_dtor(&temp);
    return;
  }

  zval_ptr_dtor(&temp);

  if (cursor->cursor_id == 0) {
    php_mongo_free_cursor_le(cursor, MONGO_CURSOR TSRMLS_CC);
  }
  // if cursor_id != 0, server should stay the same
  
  if (cursor->flag & 1) {
    zend_throw_exception(mongo_ce_CursorException, "Cursor not found", 2 TSRMLS_CC);
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

/* {{{ MongoCursor::tailable
 */
PHP_METHOD(MongoCursor, tailable) {
  zend_bool z = 1;
  preiteration_setup;
  default_to_true(1);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::dead
 */
PHP_METHOD(MongoCursor, dead) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  RETURN_BOOL(cursor->started_iterating && cursor->cursor_id == 0);
}
/* }}} */

/* {{{ MongoCursor::slaveOkay
 */
PHP_METHOD(MongoCursor, slaveOkay) {
  zend_bool z = 1;
  preiteration_setup;
  default_to_true(2);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::immortal
 */
PHP_METHOD(MongoCursor, immortal) {
  zend_bool z = 1;
  preiteration_setup;
  default_to_true(4);
  RETURN_ZVAL(getThis(), 1, 0);
}
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
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  if (cursor->started_iterating) {
    zend_throw_exception(mongo_ce_CursorException, "cannot modify cursor after beginning iteration.", 0 TSRMLS_CC);
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
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  MAKE_STD_ZVAL(snapshot);
  ZVAL_STRING(snapshot, "$snapshot", 1);
  MAKE_STD_ZVAL(yes);
  ZVAL_TRUE(yes); 

  MONGO_METHOD2(MongoCursor, addOption, return_value, getThis(), snapshot, yes);

  zval_ptr_dtor(&snapshot);
  zval_ptr_dtor(&yes);
}
/* }}} */


/* {{{ MongoCursor->sort
 */
PHP_METHOD(MongoCursor, sort) {
  zval *orderby, *fields;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &fields) == FAILURE) {
    return;
  }
  if (IS_SCALAR_P(fields)) {
    zend_error(E_WARNING, "MongoCursor::sort() expects parameter 1 to be an array or object");
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
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);
  array_init(return_value);

  add_assoc_string(return_value, "ns", cursor->ns, 1);
  add_assoc_long(return_value, "limit", cursor->limit);
  add_assoc_long(return_value, "batchSize", cursor->batch_size);
  add_assoc_long(return_value, "skip", cursor->skip);
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
    add_assoc_long(return_value, "id", cursor->cursor_id);
    add_assoc_long(return_value, "at", cursor->at);
    add_assoc_long(return_value, "numReturned", cursor->num);
    add_assoc_string(return_value, "server", cursor->server->label, 1);
  }
}
/* }}} */

/* {{{ MongoCursor->explain
 */
PHP_METHOD(MongoCursor, explain) {
  int temp_limit;
  zval *explain, *yes;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

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

  // reset to original limit
  cursor->limit = temp_limit;
}
/* }}} */


/* {{{ MongoCursor->doQuery
 */
PHP_METHOD(MongoCursor, doQuery) {
  mongo_cursor *cursor;
  buffer buf;
  zval *errmsg;

  PHP_MONGO_GET_CURSOR(getThis());

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (php_mongo_write_query(&buf, cursor TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return;
  }

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  // If slave_okay is set, read from a slave.
  if ((cursor->link->rs && cursor->opts & SLAVE_OKAY &&
       (cursor->server = php_mongo_get_slave_socket(cursor->link, errmsg TSRMLS_CC)) == 0)) {
    // ignore errors and reset errmsg
    zval_ptr_dtor(&errmsg);
    MAKE_STD_ZVAL(errmsg);
    ZVAL_NULL(errmsg);
  }

  // if getting the slave didn't work (or we're not using a rs), just get master socket
  if (cursor->server == 0 &&
      (cursor->server = php_mongo_get_socket(cursor->link, errmsg TSRMLS_CC)) == 0) {
    efree(buf.start);
    zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_P(errmsg), 14 TSRMLS_CC);
    zval_ptr_dtor(&errmsg);
    return;
  }

  if (mongo_say(cursor->server->socket, &buf, errmsg TSRMLS_CC) == FAILURE) {  
    php_mongo_disconnect_server(cursor->server);
    
    if (Z_TYPE_P(errmsg) == IS_STRING) {
      zend_throw_exception_ex(mongo_ce_CursorException, 14 TSRMLS_CC, "couldn't send query: %s", Z_STRVAL_P(errmsg));
    }
    else {
      zend_throw_exception(mongo_ce_CursorException, "couldn't send query", 14 TSRMLS_CC);
    }
    efree(buf.start);
    zval_ptr_dtor(&errmsg);
    return;
  }

  efree(buf.start);

  if (php_mongo_get_reply(cursor, errmsg TSRMLS_CC) == FAILURE) {
    zval_ptr_dtor(&errmsg);
    return;
  }

  zval_ptr_dtor(&errmsg);

  /* we've got something to kill, make a note */
  if (cursor->cursor_id != 0) {
    php_mongo_create_le(cursor, "cursor_list" TSRMLS_CC);
  }
}
/* }}} */


// ITERATOR FUNCTIONS

/* {{{ MongoCursor->current
 */
PHP_METHOD(MongoCursor, current) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

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
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

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
    RETURN_STRING("", 1);
  }
}
/* }}} */

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
    zval **err;
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

    // check for err
    if (cursor->num == 1 &&
        zend_hash_find(Z_ARRVAL_P(cursor->current), "$err", strlen("$err")+1, (void**)&err) == SUCCESS) {
      zval **code_z;
      // default error code
      int code = 4;
      
      if (zend_hash_find(Z_ARRVAL_P(cursor->current), "code", strlen("code")+1, (void**)&code_z) == SUCCESS) {
        // check for not master
        if (Z_TYPE_PP(code_z) == IS_LONG) {
          code = Z_LVAL_PP(code_z);
        }
        else if (Z_TYPE_PP(code_z) == IS_DOUBLE) {
          code = Z_DVAL_PP(code_z);
        }
        // else code == 4
        
        // this shouldn't be necessary after 1.7.* is standard, it forces
        // failover in case the master steps down.
        // not master: 10107
        // not master and slaveok=false (more recent): 13435
        // not master or secondary: 13436
        if (cursor->link->rs && (code == 10107 || code == 13435 || code == 13436)) {
          php_mongo_disconnect_server(cursor->server);
        }
      }

      zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_PP(err), code TSRMLS_CC);
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
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  RETURN_BOOL(cursor->current);
}
/* }}} */

/* {{{ MongoCursor->reset
 */
PHP_METHOD(MongoCursor, reset) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  cursor->buf.pos = cursor->buf.start;

  if (cursor->current) {
    zval_ptr_dtor(&cursor->current);
  }
  cursor->started_iterating = 0;
  cursor->current = 0;
  cursor->at = 0;
  cursor->num = 0;
  cursor->server = 0;
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

static function_entry MongoCursor_methods[] = {
  PHP_ME(MongoCursor, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hasNext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, getNext, NULL, ZEND_ACC_PUBLIC)

  /* options */
  PHP_ME(MongoCursor, limit, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, batchSize, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, skip, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, fields, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, slaveOkay, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, tailable, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, immortal, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, timeout, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, dead, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, snapshot, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, sort, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hint, NULL, ZEND_ACC_PUBLIC) 
  PHP_ME(MongoCursor, addOption, NULL, ZEND_ACC_PUBLIC)

  /* query */
  PHP_ME(MongoCursor, doQuery, NULL, ZEND_ACC_PROTECTED)

  /* iterator funcs */
  PHP_ME(MongoCursor, current, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, key, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, next, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, rewind, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, valid, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, reset, NULL, ZEND_ACC_PUBLIC)

  /* stand-alones */
  PHP_ME(MongoCursor, explain, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, count, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, info, NULL, ZEND_ACC_PUBLIC)

  {NULL, NULL, NULL}
};


static zend_object_value php_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC) {
  php_mongo_obj_new(mongo_cursor);
}


void php_mongo_cursor_free(void *object TSRMLS_DC) {
  mongo_cursor *cursor = (mongo_cursor*)object;

  if (cursor) {
    if (cursor->cursor_id != 0) {
      php_mongo_free_cursor_le(cursor, MONGO_CURSOR TSRMLS_CC);
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

