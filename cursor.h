//cursor.h
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

#ifndef MONGO_CURSOR_H
#define MONGO_CURSOR_H 1

void mongo_mongo_cursor_free(void *object TSRMLS_DC);

PHP_METHOD(MongoCursor, __construct);
PHP_METHOD(MongoCursor, getNext);
PHP_METHOD(MongoCursor, hasNext);
PHP_METHOD(MongoCursor, limit);
PHP_METHOD(MongoCursor, softLimit);
PHP_METHOD(MongoCursor, skip);
PHP_METHOD(MongoCursor, sort);
PHP_METHOD(MongoCursor, hint);
PHP_METHOD(MongoCursor, doQuery);
PHP_METHOD(MongoCursor, current);
PHP_METHOD(MongoCursor, key);
PHP_METHOD(MongoCursor, next);
PHP_METHOD(MongoCursor, rewind);
PHP_METHOD(MongoCursor, valid);
PHP_METHOD(MongoCursor, reset);
PHP_METHOD(MongoCursor, count);

#endif
