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

extern zend_class_entry *mongo_id_class,
  *mongo_ce_CursorException,
  *spl_ce_InvalidArgumentException,
  *zend_ce_iterator;

extern int le_db_cursor,
  le_connection,
  le_pconnection;

zend_class_entry *mongo_cursor_ce = NULL;


/* {{{ MongoCursor->__construct
 */
PHP_METHOD(MongoCursor, __construct) {
  zval *zlink, *zns, *zquery, *zfields;
  int param_count = 4;
  int argc = ZEND_NUM_ARGS();

  if (zend_parse_parameters(argc TSRMLS_CC, "z|zzz", &zlink, &zns, &zquery, &zfields) == FAILURE) {
    return;
  }

  zval *empty_array;
  MAKE_STD_ZVAL(empty_array);
  array_init(empty_array);

  // defaults
  zend_update_property_null(mongo_cursor_ce, getThis(), "connection", strlen("connection") TSRMLS_CC);
  zend_update_property_null(mongo_cursor_ce, getThis(), "ns", strlen("ns") TSRMLS_CC);
  zend_update_property(mongo_cursor_ce, getThis(), "query", strlen("query"), empty_array TSRMLS_CC);
  zend_update_property(mongo_cursor_ce, getThis(), "fields", strlen("fields"), empty_array TSRMLS_CC);

  switch (argc) {
  case 4:
    zend_update_property(mongo_cursor_ce, getThis(), "fields", strlen("fields"), zfields TSRMLS_CC);
  case 3:
  case 2:
    zend_update_property(mongo_cursor_ce, getThis(), "ns", strlen("ns"), zns TSRMLS_CC);
  case 1:
    zend_update_property(mongo_cursor_ce, getThis(), "connection", strlen("connection"), zlink TSRMLS_CC);
  }

  // we don't want this param to go away at the end of this method
  zval_add_ref(&zquery);

  zval *q;
  MAKE_STD_ZVAL(q);
  array_init(q);
  add_assoc_zval(q, "query", zquery);
  zend_update_property(mongo_cursor_ce, getThis(), "query", strlen("query"), q TSRMLS_CC);

  zend_update_property(mongo_cursor_ce, getThis(), "hint", strlen("hint"), empty_array TSRMLS_CC);

  zim_MongoCursor_reset(INTERNAL_FUNCTION_PARAM_PASSTHRU);

  zval_ptr_dtor(&empty_array);
  zval_ptr_dtor(&q);
}
/* }}} */

/* {{{ MongoCursor->hasNext
 */
PHP_METHOD(MongoCursor, hasNext) {
  zval *started = zend_read_property(mongo_cursor_ce, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);

  if (!Z_BVAL_P(started)) {
    zim_MongoCursor_doQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    zend_update_property_bool(mongo_cursor_ce, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);
  }

  zval *rcursor = zend_read_property(mongo_cursor_ce, getThis(), "cursor", strlen("cursor"), 1 TSRMLS_CC);
  mongo_cursor *cursor;
  ZEND_FETCH_RESOURCE(cursor, mongo_cursor*, &rcursor, -1, PHP_DB_CURSOR_RES_NAME, le_db_cursor); 

  RETURN_BOOL(mongo_do_has_next(cursor TSRMLS_CC));
}
/* }}} */

/* {{{ MongoCursor->getNext
 */
PHP_METHOD(MongoCursor, getNext) {
  zval *started = zend_read_property(mongo_cursor_ce, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);

  if (!Z_BVAL_P(started)) {
    zim_MongoCursor_doQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    zend_update_property_bool(mongo_cursor_ce, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);
  }

  zval *rcursor = zend_read_property(mongo_cursor_ce, getThis(), "cursor", strlen("cursor"), 1 TSRMLS_CC);
  mongo_cursor *cursor;
  ZEND_FETCH_RESOURCE(cursor, mongo_cursor*, &rcursor, -1, PHP_DB_CURSOR_RES_NAME, le_db_cursor); 

  zval *next = mongo_do_next(cursor TSRMLS_CC);
  if (next) {
    RETURN_ZVAL(next, 0, 1);
  }

  zval_ptr_dtor(&rcursor);
}
/* }}} */

/* {{{ MongoCursor->limit
 */
PHP_METHOD(MongoCursor, limit) {
  CHECK_AND_GET_PARAM(znum);

  convert_to_long(*znum);
  long int num = Z_LVAL_PP(znum);
  num *= -1;

  zend_update_property_long(mongo_cursor_ce, getThis(), "limit", strlen("limit"), num TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->softLimit
 */
PHP_METHOD(MongoCursor, softLimit) {
  CHECK_AND_GET_PARAM(znum);

  convert_to_long(*znum);
  long int num = Z_LVAL_PP(znum);

  zend_update_property_long(mongo_cursor_ce, getThis(), "limit", strlen("limit"), num TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->skip
 */
PHP_METHOD(MongoCursor, skip) {
  CHECK_AND_GET_PARAM(znum);

  convert_to_long(*znum);
  long int num = Z_LVAL_PP(znum);

  zend_update_property_long(mongo_cursor_ce, getThis(), "skip", strlen("skip"), num TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->sort
 */
PHP_METHOD(MongoCursor, sort) {
  CHECK_AND_GET_PARAM(zfields);

  if (Z_TYPE_PP(zfields) != IS_ARRAY) {
    zend_throw_exception(spl_ce_InvalidArgumentException, "expected MongoCursor->sort(array)", 0 TSRMLS_CC);
    return;
  }

  zval *query = zend_read_property(mongo_cursor_ce, getThis(), "query", strlen("query"), 1 TSRMLS_CC);
  zval_add_ref(zfields);
  add_assoc_zval(query, "orderby", *zfields);

  zend_update_property(mongo_cursor_ce, getThis(), "query", strlen("query"), query TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ MongoCursor->hint
 */
PHP_METHOD(MongoCursor, hint) {
  CHECK_AND_GET_PARAM(zfields);

  if (Z_TYPE_PP(zfields) != IS_ARRAY) {
    zend_throw_exception(spl_ce_InvalidArgumentException, "expected MongoCursor->hint(array)", 0 TSRMLS_CC);
    return;
  }

  zval *query = zend_read_property(mongo_cursor_ce, getThis(), "query", strlen("query"), 1 TSRMLS_CC);
  zval_add_ref(zfields);
  add_assoc_zval(query, "$hint", *zfields);

  zend_update_property(mongo_cursor_ce, getThis(), "query", strlen("query"), query TSRMLS_CC);
  RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/* {{{ MongoCursor->doQuery
 */
PHP_METHOD(MongoCursor, doQuery) {
  zval *zlink = zend_read_property(mongo_cursor_ce, getThis(), "connection", strlen("connection"), 1 TSRMLS_CC);
  zval *zquery = zend_read_property(mongo_cursor_ce, getThis(), "query", strlen("query"), 1 TSRMLS_CC);
  zval *zns = zend_read_property(mongo_cursor_ce, getThis(), "ns", strlen("ns"), 1 TSRMLS_CC);
  zval *zskip = zend_read_property(mongo_cursor_ce, getThis(), "skip", strlen("skip"), 1 TSRMLS_CC);
  zval *zlimit = zend_read_property(mongo_cursor_ce, getThis(), "limit", strlen("limit"), 1 TSRMLS_CC);
  zval *zfields = zend_read_property(mongo_cursor_ce, getThis(), "fields", strlen("fields"), 1 TSRMLS_CC);

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

  zend_update_property(mongo_cursor_ce, getThis(), "cursor", strlen("cursor"), rsrc TSRMLS_CC);

  zval_ptr_dtor(&rsrc);
}
/* }}} */


// ITERATOR FUNCTIONS

/* {{{ MongoCursor->current
 */
PHP_METHOD(MongoCursor, current) {
  zval *zcurrent = zend_read_property(mongo_cursor_ce, getThis(), "current", strlen("current"), 1 TSRMLS_CC);

  RETURN_ZVAL(zcurrent, 1, 0);
}
/* }}} */

/* {{{ MongoCursor->key
 */
PHP_METHOD(MongoCursor, key) {
  zval **id;
  zval *zcurrent = zend_read_property(mongo_cursor_ce, getThis(), "current", strlen("current"), 1 TSRMLS_CC);

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
  zval *started = zend_read_property(mongo_cursor_ce, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);

  if (!Z_BVAL_P(started)) {
    zim_MongoCursor_doQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    zend_update_property_bool(mongo_cursor_ce, getThis(), "startedIterating", strlen("startedIterating"), 1 TSRMLS_CC);
  }

  zval *current = zend_read_property(mongo_cursor_ce, getThis(), "current", strlen("current"), 1 TSRMLS_CC);

  zim_MongoCursor_hasNext(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  if (Z_BVAL_P(return_value)) {
    zval *temp = return_value;
    return_value = current;
    zim_MongoCursor_getNext(INTERNAL_FUNCTION_PARAM_PASSTHRU); 
    return_value = temp;
  }
  else if (current && Z_TYPE_P(current) != IS_NULL) {
    zend_update_property_null(mongo_cursor_ce, getThis(), "current", strlen("current") TSRMLS_CC);
  }

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
  zval *current = zend_read_property(mongo_cursor_ce, getThis(), "current", strlen("current"), 1 TSRMLS_CC);

  RETURN_BOOL(Z_TYPE_P(current) != IS_NULL);
}
/* }}} */

/* {{{ MongoCursor->reset
 */
PHP_METHOD(MongoCursor, reset) {
  zend_update_property_null(mongo_cursor_ce, getThis(), "current", strlen("current") TSRMLS_CC);
  zend_update_property_bool(mongo_cursor_ce, getThis(), "startedIterating", strlen("startedIterating"), 0 TSRMLS_CC);
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
  mongo_cursor_ce = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(mongo_cursor_ce TSRMLS_CC, 1, zend_ce_iterator);

  zend_declare_property_null(mongo_cursor_ce, "connection", strlen("connection"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_cursor_ce, "cursor", strlen("cursor"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_bool(mongo_cursor_ce, "startedIterating", strlen("startedIterating"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_cursor_ce, "current", strlen("current"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_cursor_ce, "query", strlen("query"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_cursor_ce, "fields", strlen("fields"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_long(mongo_cursor_ce, "limit", strlen("limit"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_long(mongo_cursor_ce, "skip", strlen("skip"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_cursor_ce, "ns", strlen("ns"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
