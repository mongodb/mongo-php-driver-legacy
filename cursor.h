//cursor.h
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

#ifndef MONGO_CURSOR_H
#define MONGO_CURSOR_H 1

// Cursor flags
#define TAILABLE 2
#define SLAVE_OKAY 4 
#define OPLOG_REPLAY 8
#define NO_CURSOR_TO 16
#define AWAIT_DATA 32
#define EXHAUST 64

void php_mongo_cursor_free(void *object TSRMLS_DC);

PHP_METHOD(MongoCursor, __construct);
PHP_METHOD(MongoCursor, getNext);
PHP_METHOD(MongoCursor, hasNext);
PHP_METHOD(MongoCursor, limit);
PHP_METHOD(MongoCursor, batchSize);
PHP_METHOD(MongoCursor, skip);
PHP_METHOD(MongoCursor, fields);
PHP_METHOD(MongoCursor, slaveOkay);
PHP_METHOD(MongoCursor, tailable);
PHP_METHOD(MongoCursor, immortal);
PHP_METHOD(MongoCursor, timeout);
PHP_METHOD(MongoCursor, dead);
PHP_METHOD(MongoCursor, snapshot);
PHP_METHOD(MongoCursor, sort);
PHP_METHOD(MongoCursor, hint);
PHP_METHOD(MongoCursor, addOption);
PHP_METHOD(MongoCursor, explain);
PHP_METHOD(MongoCursor, doQuery);
PHP_METHOD(MongoCursor, current);
PHP_METHOD(MongoCursor, key);
PHP_METHOD(MongoCursor, next);
PHP_METHOD(MongoCursor, rewind);
PHP_METHOD(MongoCursor, valid);
PHP_METHOD(MongoCursor, reset);
PHP_METHOD(MongoCursor, count);
PHP_METHOD(MongoCursor, info);

#define preiteration_setup   mongo_cursor *cursor;                      \
  PHP_MONGO_GET_CURSOR(getThis());                                      \
                                                                        \
  if (cursor->started_iterating) {                                      \
    zend_throw_exception(mongo_ce_CursorException,                      \
                         "cannot modify cursor after beginning iteration.", 0 \
                         TSRMLS_CC);                                    \
    return;                                                             \
  }

#define default_to_true(bit)                                            \
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &z) == FAILURE) { \
    return;                                                             \
  }                                                                     \
                                                                        \
  if (z) {                                                              \
    cursor->opts |= 1 << bit;                                           \
  } else {                                                              \
    cursor->opts &= !(1 << bit);                                        \
  }


#endif
