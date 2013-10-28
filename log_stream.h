/**
 *  Copyright 2009-2013 10gen, Inc.
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

#ifndef __LOG_STREAM_H__
#define __LOG_STREAM_H__

#include <php.h>
#include <main/php_streams.h>
#include <main/php_network.h>

#include "mcon/types.h"
#include "php_mongo.h"

#define MONGO_STREAM_NOTIFY_TYPE_IO_INIT        100
#define MONGO_STREAM_NOTIFY_TYPE_LOG            200

#define MONGO_STREAM_NOTIFY_IO_READ             111
#define MONGO_STREAM_NOTIFY_IO_WRITE            112
#define MONGO_STREAM_NOTIFY_IO_PROGRESS         PHP_STREAM_NOTIFY_PROGRESS
#define MONGO_STREAM_NOTIFY_IO_COMPLETED        PHP_STREAM_NOTIFY_COMPLETED

#define MONGO_STREAM_NOTIFY_LOG_INSERT          211
#define MONGO_STREAM_NOTIFY_LOG_QUERY           212
#define MONGO_STREAM_NOTIFY_LOG_UPDATE          213
#define MONGO_STREAM_NOTIFY_LOG_DELETE          214
#define MONGO_STREAM_NOTIFY_LOG_GETMORE         215
#define MONGO_STREAM_NOTIFY_LOG_KILLCURSOR      216
#define MONGO_STREAM_NOTIFY_LOG_BATCHINSERT     217
#define MONGO_STREAM_NOTIFY_LOG_RESPONSE_HEADER 218

void php_mongo_stream_notify_io(mongo_server_options *opts, int code, int sofar, int max TSRMLS_DC);
void mongo_log_stream_insert(mongo_connection *connection, zval *document, zval *options TSRMLS_DC);
void mongo_log_stream_query(mongo_connection *connection, mongo_cursor *cursor TSRMLS_DC);
void mongo_log_stream_update(mongo_connection *connection, zval *ns, zval *criteria, zval *newobj, zval *options, int flags TSRMLS_DC);
void mongo_log_stream_delete(mongo_connection *connection, zval *ns, zval *criteria, zval *options, int flags TSRMLS_DC);
void mongo_log_stream_getmore(mongo_connection *connection, mongo_cursor *cursor TSRMLS_DC);
void mongo_log_stream_killcursor(mongo_connection *connection, int64_t cursor_id TSRMLS_DC);
void mongo_log_stream_batchinsert(mongo_connection *connection, zval *docs, zval *options, int flags TSRMLS_DC);
void mongo_log_stream_response_header(mongo_connection *connection, mongo_cursor *cursor TSRMLS_DC);
#endif
