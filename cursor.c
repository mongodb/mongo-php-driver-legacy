//cursor.c
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

#include <php.h>
#include <zend_interfaces.h>
#include <zend_exceptions.h>

#include "mongo.h"
#include "bson.h"
#include "db.h"
#include "cursor.h"
#include "mongo_types.h"

// externs
extern zend_class_entry *mongo_ce_Id,
  *mongo_ce_DB,
  *mongo_ce_CursorException;

extern int le_connection,
  le_pconnection;

extern zend_object_handlers mongo_default_handlers;


ZEND_EXTERN_MODULE_GLOBALS(mongo);

static void kill_cursor(mongo_cursor *cursor TSRMLS_DC);
static zend_object_value mongo_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC);

zend_class_entry *mongo_ce_Cursor = NULL;

/* {{{ MongoCursor->__construct
 */
PHP_METHOD(MongoCursor, __construct) {
  zval *zlink = 0, *zns = 0, *zquery = 0, *zfields = 0, *empty_array, *q;
  mongo_cursor *cursor;
  mongo_link *link;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|zz", &zlink, &zns, &zquery, &zfields) == FAILURE) {
    return;
  }

  // if query or fields weren't passed, make them default to an empty array
  MAKE_STD_ZVAL(empty_array);
  array_init(empty_array);

  if (!zfields) {
    zfields = empty_array;
  }
  if (!zquery) {
    zquery = empty_array;
  }

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
 
  // db connection
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 
  cursor->link = link;
  cursor->resource = zlink;
  zval_add_ref(&zlink);

  // fields to return
  cursor->fields = zfields;
  zval_add_ref(&zfields);

  // ns
  convert_to_string(zns);
  cursor->ns = estrdup(Z_STRVAL_P(zns));

  // query
  MAKE_STD_ZVAL(q);
  array_init(q);
  add_assoc_zval(q, "query", zquery);
  cursor->query = q;

  // we don't want this param to go away at the end of this method
  zval_add_ref(&zquery);

  // reset iteration pointer, just in case
  MONGO_METHOD(MongoCursor, reset)(INTERNAL_FUNCTION_PARAM_PASSTHRU);

  // get rid of extra ref
  zval_ptr_dtor(&empty_array);
}
/* }}} */

/* {{{ MongoCursor::hasNext
 */
PHP_METHOD(MongoCursor, hasNext) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (!cursor->started_iterating) {
    MONGO_METHOD(MongoCursor, doQuery)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    cursor->started_iterating = 1;
  }

  RETURN_BOOL(mongo_do_has_next(cursor TSRMLS_CC));
}
/* }}} */

/* {{{ MongoCursor::getNext
 */
PHP_METHOD(MongoCursor, getNext) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (!cursor->started_iterating) {
    MONGO_METHOD(MongoCursor, doQuery)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    cursor->started_iterating = 1;
  }

  if (cursor->at >= cursor->num) {
    // check for more results
    if (!mongo_do_has_next(cursor TSRMLS_CC)) {
      // we're out of results
      RETURN_NULL();
    }
    // we got more results
  }

  if (cursor->at < cursor->num) {
    array_init(return_value);
    cursor->buf.pos = (unsigned char*)bson_to_zval(cursor->buf.pos, return_value TSRMLS_CC);

    // increment cursor position
    cursor->at++;
    return;
  }

  RETURN_NULL();
}
/* }}} */

/* {{{ MongoCursor::limit
 */
PHP_METHOD(MongoCursor, limit) {
  zval *znum;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (cursor->started_iterating) {
    zend_throw_exception(mongo_ce_CursorException, "cannot modify cursor after beginning iteration.", 0 TSRMLS_CC);
    return;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &znum) == FAILURE) {
    return;
  }
  convert_to_long(znum);

  cursor->limit = Z_LVAL_P(znum) * -1;

  RETVAL_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::softLimit
 */
PHP_METHOD(MongoCursor, softLimit) {
  zval *znum;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (cursor->started_iterating) {
    zend_throw_exception(mongo_ce_CursorException, "cannot modify cursor after beginning iteration.", 0 TSRMLS_CC);
    return;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &znum) == FAILURE) {
    return;
  }
  convert_to_long(znum);

  cursor->limit = Z_LVAL_P(znum);

  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::skip
 */
PHP_METHOD(MongoCursor, skip) {
  zval *znum;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (cursor->started_iterating) {
    zend_throw_exception(mongo_ce_CursorException, "cannot modify cursor after beginning iteration.", 0 TSRMLS_CC);
    return;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &znum) == FAILURE) {
    return;
  }
  convert_to_long(znum);

  cursor->skip = Z_LVAL_P(znum);

  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->sort
 */
PHP_METHOD(MongoCursor, sort) {
  zval *zfields, *query;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (cursor->started_iterating) {
    zend_throw_exception(mongo_ce_CursorException, "cannot modify cursor after beginning iteration.", 0 TSRMLS_CC);
    return;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &zfields) == FAILURE) {
    return;
  }

  query = cursor->query;
  zval_add_ref(&zfields);
  add_assoc_zval(query, "orderby", zfields);

  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->hint
 */
PHP_METHOD(MongoCursor, hint) {
  zval *zfields, *query;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (cursor->started_iterating) {
    zend_throw_exception(mongo_ce_CursorException, "cannot modify cursor after beginning iteration.", 0 TSRMLS_CC);
    return;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &zfields) == FAILURE) {
    return;
  }

  query = cursor->query;
  zval_add_ref(&zfields);
  add_assoc_zval(query, "$hint", zfields);

  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor->explain
 */
PHP_METHOD(MongoCursor, explain) {
  zval *query;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  MONGO_METHOD(MongoCursor, reset)(INTERNAL_FUNCTION_PARAM_PASSTHRU);

  query = cursor->query;
  add_assoc_bool(query, "$explain", 1);

  MONGO_METHOD(MongoCursor, getNext)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */


/* {{{ MongoCursor->doQuery
 */
PHP_METHOD(MongoCursor, doQuery) {
  int sent;
  mongo_msg_header header;
  mongo_cursor *cursor;
  CREATE_BUF(buf, INITIAL_BUF_SIZE);

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  CREATE_HEADER(buf, cursor->ns, strlen(cursor->ns), OP_QUERY);

  serialize_int(&buf, cursor->skip);
  serialize_int(&buf, cursor->limit);

  zval_to_bson(&buf, cursor->query, NO_PREP TSRMLS_CC);
  if (cursor->fields && zend_hash_num_elements(Z_ARRVAL_P(cursor->fields)) > 0) {
    zval_to_bson(&buf, cursor->fields, NO_PREP TSRMLS_CC);
  }

  serialize_size(buf.start, &buf);
  // sends
  sent = mongo_say(cursor->link, &buf TSRMLS_CC);
  efree(buf.start);
  if (sent == FAILURE) {
    zend_throw_exception(mongo_ce_CursorException, "couldn't send query.", 0 TSRMLS_CC);
    return;
  }

  cursor->buf.start = 0;
  cursor->buf.pos = 0;

  get_reply(cursor TSRMLS_CC);
}
/* }}} */


// ITERATOR FUNCTIONS

/* {{{ MongoCursor->current
 */
PHP_METHOD(MongoCursor, current) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
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

  if (cursor->current && 
      Z_TYPE_P(cursor->current) == IS_ARRAY &&
      zend_hash_find(Z_ARRVAL_P(cursor->current), "_id", 4, (void**)&id) == SUCCESS) {
    MONGO_METHOD(MongoId, __toString)(0, return_value, return_value_ptr, *id, return_value_used TSRMLS_CC);
  }
  else {
    RETURN_STRING("", 1);
  }
}
/* }}} */

/* {{{ MongoCursor->next
 */
PHP_METHOD(MongoCursor, next) {
  mongo_cursor *cursor;

  return_value_ptr = &return_value;

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  if (!cursor->started_iterating) {
    MONGO_METHOD(MongoCursor, doQuery)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    cursor->started_iterating = 1;
  }

  if (cursor->current) {
    zval_ptr_dtor(&cursor->current);
    cursor->current = 0;
  }

  MONGO_METHOD(MongoCursor, hasNext)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  if (Z_BVAL_P(return_value)) {
    MAKE_STD_ZVAL(cursor->current);
    MONGO_METHOD(MongoCursor, getNext)(0, cursor->current, &cursor->current, getThis(), return_value_used TSRMLS_CC); 
  }

  RETURN_NULL();
}
/* }}} */

/* {{{ MongoCursor->rewind
 */
PHP_METHOD(MongoCursor, rewind) {
  MONGO_METHOD(MongoCursor, reset)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  MONGO_METHOD(MongoCursor, next)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ MongoCursor->valid
 */
PHP_METHOD(MongoCursor, valid) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  RETURN_BOOL(cursor->current);
}
/* }}} */

/* {{{ MongoCursor->reset
 */
PHP_METHOD(MongoCursor, reset) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  cursor->started_iterating = 0;
  cursor->current = 0;
}
/* }}} */

PHP_METHOD(MongoCursor, count) {
  zval *response, *data, *db;
  zval **n;
  mongo_cursor *cursor;
  mongo_db *db_struct;

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  // fake a MongoDB object
  MAKE_STD_ZVAL(db);
  object_init_ex(db, mongo_ce_DB);
  db_struct = (mongo_db*)zend_object_store_get_object(db TSRMLS_CC);

  db_struct->link = cursor->resource;
  zval_add_ref(&cursor->resource);
  MAKE_STD_ZVAL(db_struct->name);
  ZVAL_STRING(db_struct->name, estrndup(cursor->ns, strchr(cursor->ns, '.') - cursor->ns), 0);

  // create query
  MAKE_STD_ZVAL(data);
  array_init(data);
  add_assoc_string(data, "count", strchr(cursor->ns, '.')+1, 1);
  if (cursor->query) {
    zval **inner_query;
    if (zend_hash_find(Z_ARRVAL_P(cursor->query), "query", strlen("query")+1, (void**)&inner_query) == SUCCESS) {
      add_assoc_zval(data, "query", *inner_query);
      zval_add_ref(inner_query);
    }
  }
  if (cursor->fields) {
    add_assoc_zval(data, "fields", cursor->fields);
    zval_add_ref(&cursor->fields);
  }

  MAKE_STD_ZVAL(response);

  PUSH_PARAM(data); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoDB, command)(1, response, &response, db, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&data);
  if (zend_hash_find(Z_ARRVAL_P(response), "n", 2, (void**)&n) == SUCCESS) {
    RETVAL_ZVAL(*n, 1, 0);
    zval_ptr_dtor(&response);
  }
  else {
    RETURN_ZVAL(response, 0, 0);
  }
  zend_objects_store_del_ref(db TSRMLS_CC);
  zval_ptr_dtor(&db);
}

static function_entry MongoCursor_methods[] = {
  PHP_ME(MongoCursor, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hasNext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, getNext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, limit, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, softLimit, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, skip, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, sort, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hint, NULL, ZEND_ACC_PUBLIC) 
  PHP_ME(MongoCursor, explain, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, doQuery, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(MongoCursor, current, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, key, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, next, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, rewind, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, valid, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, reset, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, count, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};


static zend_object_value mongo_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC) {
  zend_object_value retval;
  mongo_cursor *intern;
  zval *tmp;

  intern = (mongo_cursor*)emalloc(sizeof(mongo_cursor));
  memset(intern, 0, sizeof(mongo_cursor));

  zend_object_std_init(&intern->std, class_type TSRMLS_CC);
  zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, 
                 (void *) &tmp, sizeof(zval *));

  retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, mongo_mongo_cursor_free, NULL TSRMLS_CC);
  retval.handlers = &mongo_default_handlers;

  return retval;
}


// tell db to destroy its cursor
static void kill_cursor(mongo_cursor *cursor TSRMLS_DC) {
  unsigned char quickbuf[128];
  buffer buf;
  mongo_msg_header header;

  // we allocate a cursor even if no results are returned,
  // but the database will throw an assertion if we try to
  // kill a non-existant cursor
  // non-cursors have ids of 0
  if (cursor->cursor_id == 0) {
    return;
  }
  buf.pos = quickbuf;
  buf.start = buf.pos;
  buf.end = buf.start + 128;

  // std header
  CREATE_MSG_HEADER(MonGlo(request_id)++, 0, OP_KILL_CURSORS);
  APPEND_HEADER(buf);

  // # of cursors
  serialize_int(&buf, 1);
  // cursor ids
  serialize_long(&buf, cursor->cursor_id);
  serialize_size(buf.start, &buf);

  mongo_say(cursor->link, &buf TSRMLS_CC);
}

void mongo_mongo_cursor_free(void *object TSRMLS_DC) {
  mongo_cursor *cursor = (mongo_cursor*)object;

  if (cursor) {
    kill_cursor(cursor TSRMLS_CC);

    if (cursor->current) zval_ptr_dtor(&cursor->current);

    if (cursor->query) zval_ptr_dtor(&cursor->query);
    if (cursor->fields) zval_ptr_dtor(&cursor->fields);

    if (cursor->resource) zval_ptr_dtor(&cursor->resource);
 
    if (cursor->buf.start) efree(cursor->buf.start);
    if (cursor->ns) efree(cursor->ns);

    zend_object_std_dtor(&cursor->std TSRMLS_CC);

    efree(cursor);
  }
}

void mongo_init_MongoCursor(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoCursor", MongoCursor_methods);
  ce.create_object = mongo_mongo_cursor_new;
  mongo_ce_Cursor = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(mongo_ce_Cursor TSRMLS_CC, 1, zend_ce_iterator);
}

