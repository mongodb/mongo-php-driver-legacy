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
#define BSON_LONG 16


int serialize_element(char*, int*, char*, int, zval** TSRMLS_DC);
int serialize_double(char*, int*, double);
int serialize_string(char*, int*, char*, int);
int serialize_long(char*, int*, long);
int serialize_int(char*, int*, int);

int serialize_size(char*, int, int);

int serialize_byte(char*, int*, char);
#define set_type(buf, pos, type) serialize_byte(buf, pos, (char)type)
#define serialize_null(buf, pos) serialize_byte(buf, pos, (char)0)
#define serialize_bool(buf, pos, b) serialize_byte(buf, pos, (char)b)

int zval_to_bson(char*, int*, HashTable* TSRMLS_DC);
char* bson_to_zval(char*, zval* TSRMLS_DC);

int php_array_to_bson(mongo::BSONObjBuilder*, HashTable* TSRMLS_DC);
void bson_to_php_array(mongo::BSONObj*, zval* TSRMLS_DC);
int prep_obj_for_db(mongo::BSONObjBuilder*, HashTable* TSRMLS_DC);
int add_to_bson(mongo::BSONObjBuilder*, char*, zval** TSRMLS_DC);
