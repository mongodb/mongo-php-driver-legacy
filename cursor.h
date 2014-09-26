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
#ifndef MONGO_CURSOR_H
#define MONGO_CURSOR_H 1

#include "php_mongo.h"

int php_mongocursor_create(mongo_cursor *cursor, zval *zlink, char *ns, int ns_len, zval *zquery, zval *zfields TSRMLS_DC);
void php_mongo_runquery(mongo_cursor *cursor TSRMLS_DC);
int php_mongocursor_is_valid(mongo_cursor *cursor);
int php_mongocursor_load_current_element(mongo_cursor *cursor TSRMLS_DC);
int php_mongocursor_advance(mongo_cursor *cursor TSRMLS_DC);

PHP_METHOD(MongoCursor, __construct);
PHP_METHOD(MongoCursor, getNext);
PHP_METHOD(MongoCursor, hasNext);
PHP_METHOD(MongoCursor, limit);
PHP_METHOD(MongoCursor, skip);
PHP_METHOD(MongoCursor, fields);

PHP_METHOD(MongoCursor, setFlag);
PHP_METHOD(MongoCursor, tailable);
PHP_METHOD(MongoCursor, slaveOkay);
PHP_METHOD(MongoCursor, immortal);
PHP_METHOD(MongoCursor, awaitData);
PHP_METHOD(MongoCursor, partial);

PHP_METHOD(MongoCursor, timeout);
PHP_METHOD(MongoCursor, snapshot);
PHP_METHOD(MongoCursor, sort);
PHP_METHOD(MongoCursor, hint);

PHP_METHOD(MongoCursor, getReadPreference);
PHP_METHOD(MongoCursor, setReadPreference);

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

void mongo_init_MongoCursor(TSRMLS_D);
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
