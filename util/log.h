// log.h
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

#ifndef MONGO_LOG_H
#define MONGO_LOG_H

#define MONGO_LOG_NONE 0

#define MONGO_LOG_WARNING 1
#define MONGO_LOG_INFO 2
#define MONGO_LOG_FINE 4

#define MONGO_LOG_RS 1
#define MONGO_LOG_POOL 2
#define MONGO_LOG_IO 4
#define MONGO_LOG_SERVER 8
#define MONGO_LOG_PARSE 4
#define MONGO_LOG_ALL 15

void mongo_init_MongoLog(TSRMLS_D);

PHP_METHOD(MongoLog, setLevel);
PHP_METHOD(MongoLog, getLevel);
PHP_METHOD(MongoLog, setModule);
PHP_METHOD(MongoLog, getModule);

void mongo_log(const int module, const int level TSRMLS_DC, const char *format, ...);

#endif
