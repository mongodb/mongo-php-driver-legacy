// io.h
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

#ifndef MONGO_IO_H
#define MONGO_IO_H

/*
 * Internal functions
 */
int mongo_say(mongo_server *server, buffer *buf, zval *errmsg TSRMLS_DC);
int _mongo_say(int sock, buffer *buf, zval *errmsg TSRMLS_DC);
/**
 * If there was an error, set EG(exception) and return FAILURE. If the socket
 * was closed, return FAILURE (without setting the exception).
 */
int mongo_hear(int sock, void*, int TSRMLS_DC);
int php_mongo_get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC);
int php_mongo__get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC);

#endif
