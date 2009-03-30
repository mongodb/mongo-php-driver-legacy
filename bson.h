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
#define BSON_INT 16

#define GROW_SLOWLY 1048576

char* serialize_size(char*, char*);

char* serialize_element(char*, char*, char*, int, zval** TSRMLS_DC);
char* serialize_double(char*, char*, double);
char* serialize_string(char*, char*, char*, int);
char* serialize_long(char*, char*, long);
char* serialize_int(char*, char*, int);
char* serialize_byte(char*, char*, char);

#define set_type(buf, size, type) serialize_byte(buf, size, (char)type)
#define serialize_null(buf, size) serialize_byte(buf, size, (char)0)
#define serialize_bool(buf, size, b) serialize_byte(buf, size, (char)b)

char* resize_buf(char*, char*);

char* prep_obj_for_db(char*, char*, HashTable* TSRMLS_DC);
char* zval_to_bson(char*, char*, HashTable* TSRMLS_DC);
char* bson_to_zval(char*, zval* TSRMLS_DC);
