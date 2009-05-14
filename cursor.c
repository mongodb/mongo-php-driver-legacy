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

#include "cursor.h"
#include "mongo.h"
#include "mongo_types.h"

extern zend_class_entry *mongo_ce_Id,
  *mongo_ce_CursorException,
  *zend_ce_iterator;

extern int le_db_cursor,
  le_connection,
  le_pconnection;

zend_class_entry *mongo_ce_Cursor = NULL;


/* {{{ MongoCursor->__construct
 */
PHP_METHOD(MongoCursor, __construct) {
  zval *zlink = 0, *zns = 0, *zquery = 0, *zfields = 0;
  int argc = ZEND_NUM_ARGS();

  if (zend_parse_parameters(argc TSRMLS_CC, "zz|zz", &zlink, &zns, &zquery, &zfields) == FAILURE) {
    return;
  }

  zval *empty_array;
  MAKE_STD_ZVAL(empty_array);
  array_init(empty_array);

  if (!zfields) {
    zfields = empty_array;
  }
  if (!zquery) {
    zquery = empty_array;
  }

  zend_update_property(mongo_ce_Cursor, getThis(), "fields", strlen("fields"), zfields TSRMLS_CC);
  zend_update_property(mongo_ce_Cursor, getThis(), "ns", strlen("ns"), zns TSRMLS_CC);
  zend_update_property(mongo_ce_Cursor, getThis(), "connection", strlen("connection"), zlink TSRMLS_CC);

  // we don't want this param to go away at the end of this method
  zval_add_ref(&zquery);

  zval *q;
  MAKE_STD_ZVAL(q);
  array_init(q);
  add_assoc_zval(q, "query", zquery);
  zend_update_property(mongo_ce_Cursor, getThis(), "query", strlen("query"), q TSRMLS_CC);

  zend_update_property(mongo_ce_Cursor, getThis(), "hint", strlen("hint"), empty_array TSRMLS_CC);

  zim_MongoCursor_reset(INTERNAL_FUNCTION_PARAM_PASSTHRU);

  // get rid of extra refs
  zval_ptr_dtor(&empty_array);
  zval_ptr_dtor(&q);
}
/* }}} */

/* {{{ MongoCursor::hasNext
 */
PHP_METHOD(MongoCursor, hasNext) {
  zval *started = zend_read_property(mongo_ce_Cursor, getThis(), "startedIterating", strlen("startedIterating"), NOISY TSRMLS_CC);

  if (!Z_BVAL_P(started)) {
    zim_MongoCursor_doQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    zend_update_property_bool(mongo_ce_Cursor, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);
  }

  zval *rcursor = zend_read_property(mongo_ce_Cursor, getThis(), "cursor", strlen("cursor"), 1 TSRMLS_CC);
  mongo_cursor *cursor;
  ZEND_FETCH_RESOURCE(cursor, mongo_cursor*, &rcursor, -1, PHP_DB_CURSOR_RES_NAME, le_db_cursor); 

  RETURN_BOOL(mongo_do_has_next(cursor TSRMLS_CC));
}
/* }}} */

/* {{{ MongoCursor::getNext
 */
PHP_METHOD(MongoCursor, getNext) {
  zval *started = zend_read_property(mongo_ce_Cursor, getThis(), "startedIterating", strlen("startedIterating"), NOISY TSRMLS_CC);

  if (!Z_BVAL_P(started)) {
    zim_MongoCursor_doQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    zend_update_property_bool(mongo_ce_Cursor, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);
  }

  zval *rcursor = zend_read_property(mongo_ce_Cursor, getThis(), "cursor", strlen("cursor"), 1 TSRMLS_CC);
  mongo_cursor *cursor;
  ZEND_FETCH_RESOURCE(cursor, mongo_cursor*, &rcursor, -1, PHP_DB_CURSOR_RES_NAME, le_db_cursor); 

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
    cursor->buf.pos = bson_to_zval(cursor->buf.pos, return_value TSRMLS_CC);

    // increment cursor position
    cursor->at++;
    return;
  }

  RETURN_NULL();
}
/* }}} */

/* {{{ MongoCursor->limit
 */
PHP_METHOD(MongoCursor, limit) {
  long int num;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &num) == FAILURE) {
    return;
  }
  num *= -1;

  zend_update_property_long(mongo_ce_Cursor, getThis(), "limit", strlen("limit"), num TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->softLimit
 */
PHP_METHOD(MongoCursor, softLimit) {
  zval *znum;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &znum) == FAILURE) {
    return;
  }
  convert_to_long(znum);

  zend_update_property(mongo_ce_Cursor, getThis(), "limit", strlen("limit"), znum TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->skip
 */
PHP_METHOD(MongoCursor, skip) {
  zval *znum;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &znum) == FAILURE) {
    return;
  }
  convert_to_long(znum);

  zend_update_property(mongo_ce_Cursor, getThis(), "skip", strlen("skip"), znum TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->sort
 */
PHP_METHOD(MongoCursor, sort) {
  zval *zfields;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &zfields) == FAILURE) {
    return;
  }

  zval *query = zend_read_property(mongo_ce_Cursor, getThis(), "query", strlen("query"), 1 TSRMLS_CC);
  zval_add_ref(&zfields);
  add_assoc_zval(query, "orderby", zfields);

  zend_update_property(mongo_ce_Cursor, getThis(), "query", strlen("query"), query TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->hint
 */
PHP_METHOD(MongoCursor, hint) {
  zval *zfields;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &zfields) == FAILURE) {
    return;
  }

  zval *query = zend_read_property(mongo_ce_Cursor, getThis(), "query", strlen("query"), 1 TSRMLS_CC);
  zval_add_ref(&zfields);
  add_assoc_zval(query, "$hint", zfields);

  zend_update_property(mongo_ce_Cursor, getThis(), "query", strlen("query"), query TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor->doQuery
 */
PHP_METHOD(MongoCursor, doQuery) {
  zval *zlink = zend_read_property(mongo_ce_Cursor, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  zval *zquery = zend_read_property(mongo_ce_Cursor, getThis(), "query", strlen("query"), 1 TSRMLS_CC);
  zval *zns = zend_read_property(mongo_ce_Cursor, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);
  zval *zskip = zend_read_property(mongo_ce_Cursor, getThis(), "skip", strlen("skip"), 1 TSRMLS_CC);
  zval *zlimit = zend_read_property(mongo_ce_Cursor, getThis(), "limit", strlen("limit"), 1 TSRMLS_CC);
  zval *zfields = zend_read_property(mongo_ce_Cursor, getThis(), "fields", strlen("fields"), 1 TSRMLS_CC);

  mongo_link *link;
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zlink, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  zval **data;
  zend_hash_find(Z_ARRVAL_P(zquery), "query", 6, (void**)&data);

  convert_to_string(zns);
  convert_to_long(zskip);
  convert_to_long(zlimit);

  mongo_cursor *cursor = mongo_do_query(link, 
                                        Z_STRVAL_P(zns),
                                        Z_LVAL_P(zskip),
                                        Z_LVAL_P(zlimit),
                                        zquery,
                                        zfields
                                        TSRMLS_CC);


  zval *rsrc;
  MAKE_STD_ZVAL(rsrc);
  ZEND_REGISTER_RESOURCE(rsrc, cursor, le_db_cursor);

  zend_update_property(mongo_ce_Cursor, getThis(), "cursor", strlen("cursor"), rsrc TSRMLS_CC);

  zval_ptr_dtor(&rsrc);
}
/* }}} */


// ITERATOR FUNCTIONS

/* {{{ MongoCursor->current
 */
PHP_METHOD(MongoCursor, current) {
  zval *zcurrent = zend_read_property(mongo_ce_Cursor, getThis(), "current", strlen("current"), NOISY TSRMLS_CC);
  RETURN_ZVAL(zcurrent, 1, 0);
}
/* }}} */

/* {{{ MongoCursor->key
 */
PHP_METHOD(MongoCursor, key) {
  zval **id;
  zval *zcurrent = zend_read_property(mongo_ce_Cursor, getThis(), "current", strlen("current"), 1 TSRMLS_CC);

  if (Z_TYPE_P(zcurrent) == IS_ARRAY &&
      zend_hash_find(Z_ARRVAL_P(zcurrent), "_id", 4, (void**)&id) == SUCCESS) {
    return_value_ptr = &return_value;
    zval *temp_this = this_ptr;
    this_ptr = *id;
    zim_MongoId___toString(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    this_ptr = temp_this;
  }
  else {
    RETURN_STRING("", 1);
  }
}
/* }}} */

/* {{{ MongoCursor->next
 */
PHP_METHOD(MongoCursor, next) {
  return_value_ptr = &return_value;
  zval *started = zend_read_property(mongo_ce_Cursor, getThis(), "startedIterating", strlen("startedIterating"), NOISY TSRMLS_CC);

  if (!Z_BVAL_P(started)) {
    zim_MongoCursor_doQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    zend_update_property_bool(mongo_ce_Cursor, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);
  }

  zval *current = zend_read_property(mongo_ce_Cursor, getThis(), "current", strlen("current"), NOISY TSRMLS_CC);

  zim_MongoCursor_hasNext(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  if (Z_BVAL_P(return_value)) {
    zval *next;
    MAKE_STD_ZVAL(next);

    zim_MongoCursor_getNext(0, next, &next, getThis(), return_value_used TSRMLS_CC); 

    zend_update_property(mongo_ce_Cursor, getThis(), "current", strlen("current"), next TSRMLS_CC);
  }
  else {
    zend_update_property_null(mongo_ce_Cursor, getThis(), "current", strlen("current") TSRMLS_CC);
  }
  zval_ptr_dtor(&current);

  RETURN_NULL();
}
/* }}} */

/* {{{ MongoCursor->rewind
 */
PHP_METHOD(MongoCursor, rewind) {
  zim_MongoCursor_reset(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  zim_MongoCursor_next(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ MongoCursor->valid
 */
PHP_METHOD(MongoCursor, valid) {
  zval *current = zend_read_property(mongo_ce_Cursor, getThis(), "current", strlen("current"), NOISY TSRMLS_CC);

  RETURN_BOOL(Z_TYPE_P(current) != IS_NULL);
}
/* }}} */

/* {{{ MongoCursor->reset
 */
PHP_METHOD(MongoCursor, reset) {
  zend_update_property_null(mongo_ce_Cursor, getThis(), "current", strlen("current") TSRMLS_CC);
  zend_update_property_bool(mongo_ce_Cursor, getThis(), "startedIterating", strlen("startedIterating"), 0 TSRMLS_CC);
}
/* }}} */

static function_entry MongoCursor_methods[] = {
  PHP_ME(MongoCursor, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hasNext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, getNext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, limit, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, softLimit, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, skip, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, sort, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, hint, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, doQuery, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(MongoCursor, current, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, key, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, next, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, rewind, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, valid, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoCursor, reset, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

void mongo_init_MongoCursor(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoCursor", MongoCursor_methods);
  mongo_ce_Cursor = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(mongo_ce_Cursor TSRMLS_CC, 1, zend_ce_iterator);

  zend_declare_property_null(mongo_ce_Cursor, "connection", strlen("connection"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Cursor, "cursor", strlen("cursor"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_bool(mongo_ce_Cursor, "startedIterating", strlen("startedIterating"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Cursor, "current", strlen("current"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Cursor, "query", strlen("query"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Cursor, "fields", strlen("fields"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_long(mongo_ce_Cursor, "limit", strlen("limit"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_long(mongo_ce_Cursor, "skip", strlen("skip"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_Cursor, "ns", strlen("ns"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
