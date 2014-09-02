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
#ifndef __TYPES_DATE_H__
#define __TYPES_DATE_H__

PHP_METHOD(MongoDate, __construct);
PHP_METHOD(MongoDate, __toString);
PHP_METHOD(MongoDate, __set_state);
PHP_METHOD(MongoDate, toDateTime);

void php_mongo_mongodate_populate(zval *mongocode_object, long sec, long usec TSRMLS_DC);
void php_mongo_mongodate_make_now(long *sec, long *usec);

void php_mongo_date_init(zval *value, int64_t datetime TSRMLS_DC);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
