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

#ifndef PHP_MONGO_TYPES_H
#define PHP_MONGO_TYPES_H 1

#define BIN_FUNCTION 1
#define BIN_BYTE_ARRAY 2
#define BIN_UUID 3
#define BIN_MD5 5
#define BIN_CUSTOM 128

PHP_METHOD(MongoBinData, __construct);
PHP_METHOD(MongoBinData, __toString);

PHP_METHOD(MongoCode, __construct );
PHP_METHOD(MongoCode, __toString );

PHP_METHOD(MongoDate, __construct);
PHP_METHOD(MongoDate, __toString);

void generate_id(char* TSRMLS_DC);
PHP_METHOD(MongoId, __construct);
PHP_METHOD(MongoId, __toString);
PHP_METHOD(MongoId, __set_state);
PHP_METHOD(MongoId, getTimestamp);
PHP_METHOD(MongoId, getHostname);

PHP_METHOD(MongoInt32, __construct);
PHP_METHOD(MongoInt32, __toString);

PHP_METHOD(MongoInt64, __construct);
PHP_METHOD(MongoInt64, __toString);

int php_mongo_id_serialize(zval*, unsigned char**, zend_uint*, zend_serialize_data* TSRMLS_DC);
int php_mongo_id_unserialize(zval**, zend_class_entry*, const unsigned char*, zend_uint, zend_unserialize_data* TSRMLS_DC);
int php_mongo_compare_ids(zval*, zval* TSRMLS_DC);

PHP_METHOD(MongoRegex, __construct);
PHP_METHOD(MongoRegex, __toString);

PHP_METHOD(MongoDBRef, create);
PHP_METHOD(MongoDBRef, isRef);
PHP_METHOD(MongoDBRef, get);

PHP_METHOD(MongoTimestamp, __construct);
PHP_METHOD(MongoTimestamp, __toString);

#endif

