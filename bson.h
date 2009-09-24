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
#ifndef PHP_BSON_H
#define PHP_BSON_H 1


#define INT_32 4
#define INT_64 8
#define DOUBLE_64 8
#define BYTE_8 1
#define OID_SIZE 12

#define BSON_DOUBLE 1
#define BSON_STRING 2
#define BSON_OBJECT 3
#define BSON_ARRAY 4
#define BSON_BINARY 5
#define BSON_UNDEF 6
#define BSON_OID 7
#define BSON_BOOL 8
#define BSON_DATE 9
#define BSON_NULL 10
#define BSON_REGEX 11
#define BSON_DBREF 12
#define BSON_CODE__D 13
#define BSON_CODE 15
#define BSON_INT 16
#define BSON_TIMESTAMP 17
#define BSON_LONG 18
#define BSON_MINKEY -1
#define BSON_MAXKEY 127

#define GROW_SLOWLY 1048576

void php_mongo_serialize_size(unsigned char*, buffer*);


int php_mongo_serialize_element(char*, zval**, buffer*, int TSRMLS_DC);
void php_mongo_serialize_double(buffer*, double);
void php_mongo_serialize_string(buffer*, char*, int);
void php_mongo_serialize_long(buffer*, long long);
void php_mongo_serialize_int(buffer*, int);
void php_mongo_serialize_byte(buffer*, char);
void php_mongo_serialize_bytes(buffer*, char*, int);
void php_mongo_serialize_key(buffer*, char*, int, int TSRMLS_DC);
void php_mongo_serialize_ns(buffer*, char* TSRMLS_DC);

#define php_mongo_set_type(buf, type) php_mongo_serialize_byte(buf, (char)type)
#define php_mongo_serialize_null(buf) php_mongo_serialize_byte(buf, (char)0)
#define php_mongo_serialize_bool(buf, b) php_mongo_serialize_byte(buf, (char)b)

int resize_buf(buffer*, int);

int zval_to_bson(buffer*, HashTable*, int TSRMLS_DC);
char* bson_to_zval(char*, HashTable* TSRMLS_DC);

#endif
