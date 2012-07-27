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

void mongo_init_MongoLog(TSRMLS_D);

PHP_METHOD(MongoLog, setLevel);
PHP_METHOD(MongoLog, getLevel);
PHP_METHOD(MongoLog, setModule);
PHP_METHOD(MongoLog, getModule);

void php_mongo_log(const int module, const int level TSRMLS_DC, const char *format, ...);
void php_mcon_log_wrapper(int module, int level, void *context, char *format, va_list arg);

#endif
