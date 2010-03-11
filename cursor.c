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
#include "mongo_types.h"

// externs
extern zend_class_entry *mongo_ce_Id,
  *mongo_ce_Mongo,
  *mongo_ce_DB,
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
  zval *zlink = 0, *zns = 0, *zquery = 0, *zfields = 0, *empty, *slave_okay;
  zval **data;
  mongo_cursor *cursor;
  mongo_link *link;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz|zz", &zlink, mongo_ce_Mongo, &zns, &zquery, &zfields) == FAILURE) {
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
  link = (mongo_link*)zend_object_store_get_object(zlink TSRMLS_CC);
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

      if (key_type == HASH_KEY_IS_LONG &&
          Z_TYPE_PP(data) == IS_STRING) {
        add_assoc_long(fields, Z_STRVAL_PP(data), 1);
      }
      else if (IS_SCALAR_PP(data)) {
        add_assoc_long(fields, key, Z_LVAL_PP(data));
      }
      else {
        add_assoc_long(fields, key, 1);
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
  cursor->timeout = 0;

  slave_okay = zend_read_static_property(mongo_ce_Cursor, "slaveOkay", strlen("slaveOkay"), NOISY TSRMLS_CC);
  cursor->opts = Z_BVAL_P(slave_okay) ? (1 << 2) : 0;

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
  add_assoc_zval(cursor->query, "query", temp);
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

  // we have to go and check with the db
  size = 34+strlen(cursor->ns);
  CREATE_BUF(buf, size);
  if (FAILURE == php_mongo_write_get_more(&buf, cursor TSRMLS_CC)) {
    efree(buf.start);
    return;
  }

  // fails if we're out of elems
  MAKE_STD_ZVAL(temp);
  ZVAL_NULL(temp);
  if(mongo_say(cursor->link, &buf, temp TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_P(temp), 0 TSRMLS_CC);
    zval_ptr_dtor(&temp);
    return;
  }

  efree(buf.start);

  if (php_mongo_get_reply(cursor, temp TSRMLS_CC) != SUCCESS) {
    if (Z_TYPE_P(temp) == IS_STRING) {
      zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_P(temp), 0 TSRMLS_CC);
    }
    else {
      RETURN_FALSE;
    }
    zval_ptr_dtor(&temp);
    return;
  }

  zval_ptr_dtor(&temp);

  RETURN_TRUE;

}
/* }}} */

/* {{{ MongoCursor::getNext
 */
PHP_METHOD(MongoCursor, getNext) {
  MONGO_METHOD(MongoCursor, next, return_value, getThis());
  // will be null unless there was an error
  if (Z_TYPE_P(return_value) == IS_BOOL &&
      Z_BVAL_P(return_value) == 0) {
    return;
  }
  MONGO_METHOD(MongoCursor, current, return_value, getThis());
}
/* }}} */

/* {{{ MongoCursor::limit
 */
PHP_METHOD(MongoCursor, limit) {
  preiteration_setup;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z) == FAILURE) {
    return;
  }
  convert_to_long(z);

  cursor->limit = Z_LVAL_P(z);
  RETVAL_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor::skip
 */
PHP_METHOD(MongoCursor, skip) {
  preiteration_setup;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z) == FAILURE) {
    return;
  }
  convert_to_long(z);

  cursor->skip = Z_LVAL_P(z);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::tailable
 */
PHP_METHOD(MongoCursor, tailable) {
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
  preiteration_setup;
  default_to_true(2);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::immortal
 */
PHP_METHOD(MongoCursor, immortal) {
  preiteration_setup;
  default_to_true(4);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor::timeout
 */
PHP_METHOD(MongoCursor, timeout) {
  int timeout;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &timeout) == FAILURE) {
    return;
  }

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
  ZVAL_STRING(orderby, "orderby", 1);

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

/* {{{ MongoCursor->getQuery: Return information about the current query (by @crodas)
 */
PHP_METHOD(MongoCursor, getQuery)
{
    mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
    MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);
    array_init(return_value);

    add_assoc_string(return_value, "ns", cursor->ns, 1);
    add_assoc_long(return_value, "limit", cursor->limit);
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
  int sent;
  mongo_cursor *cursor;
  buffer buf;
  zval *errmsg;

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  if (php_mongo_write_query(&buf, cursor TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return;
  }

  MAKE_STD_ZVAL(errmsg);
  ZVAL_NULL(errmsg);

  if (mongo_say(cursor->link, &buf, errmsg TSRMLS_CC) == FAILURE) {
    zend_throw_exception(mongo_ce_CursorException, "couldn't send query.", 0 TSRMLS_CC);
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
  php_mongo_create_le(cursor TSRMLS_CC);
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

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

  if (!cursor->started_iterating) {
    MONGO_METHOD(MongoCursor, doQuery, return_value, getThis());
    cursor->started_iterating = 1;
  }

  // destroy old current
  if (cursor->current) {
    zval_ptr_dtor(&cursor->current);
    cursor->current = 0;
  }

  // check for results
  MONGO_METHOD(MongoCursor, hasNext, &has_next, getThis());
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

    if (cursor->buf.pos == 0) {
      zend_throw_exception(mongo_ce_CursorException, "error parsing bson", 0 TSRMLS_CC);
      RETURN_FALSE;
    }

    // increment cursor position
    cursor->at++;

    // check for err
    if (cursor->num == 1 &&
        zend_hash_find(Z_ARRVAL_P(cursor->current), "$err", 5, (void**)&err) == SUCCESS) {
      zend_throw_exception(mongo_ce_CursorException, Z_STRVAL_PP(err), 0 TSRMLS_CC);
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
}
/* }}} */

PHP_METHOD(MongoCursor, count) {
  zval *response, *data, *db;
  zval **n;
  mongo_cursor *cursor;
  mongo_db *db_struct;
  zend_bool all = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &all) == FAILURE) {
    return;
  }

  cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoCursor);

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

  // "count" => "collectionName"
  add_assoc_string(data, "count", strchr(cursor->ns, '.')+1, 1);

  if (cursor->query) {
    zval **inner_query;

    if (!cursor->special) {
      add_assoc_zval(data, "query", cursor->query);
      zval_add_ref(&cursor->query);
    }
    else if (zend_hash_find(HASH_P(cursor->query), "query", strlen("query")+1, (void**)&inner_query) == SUCCESS) {
      add_assoc_zval(data, "query", *inner_query);
      zval_add_ref(inner_query);
    }

    /*
     * "all" creates a count based on what the cursor is actually going to return,
     * including the query, sort, limit, and skip.
     */
    if (all) {
      /* make the limit a hard limit */
      add_assoc_long(data, "limit", cursor->limit);
      add_assoc_long(data, "skip", cursor->skip);
    }
  }

  MAKE_STD_ZVAL(response);
  MONGO_CMD(response, db);

  zval_ptr_dtor(&data);

  // prep results
  if (zend_hash_find(HASH_P(response), "n", 2, (void**)&n) == SUCCESS) {
    convert_to_long(*n);
    RETVAL_ZVAL(*n, 1, 0);
    zval_ptr_dtor(&response);
  }
  else {
    RETVAL_ZVAL(response, 0, 0);
  }

  zval_ptr_dtor(&db);
}

static function_entry MongoCursor_methods[] = {
  PHP_ME(MongoCursor, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hasNext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, getNext, NULL, ZEND_ACC_PUBLIC)

  /* options */
  PHP_ME(MongoCursor, limit, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, skip, NULL, ZEND_ACC_PUBLIC)
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
  PHP_ME(MongoCursor, getQuery, NULL, ZEND_ACC_PUBLIC)

  {NULL, NULL, NULL}
};


static zend_object_value php_mongo_cursor_new(zend_class_entry *class_type TSRMLS_DC) {
  php_mongo_obj_new(mongo_cursor);
}


void php_mongo_cursor_free(void *object TSRMLS_DC) {
  mongo_cursor *cursor = (mongo_cursor*)object;

  if (cursor) {
    php_mongo_free_cursor_le(cursor, MONGO_CURSOR TSRMLS_CC);

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
}

