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

#include <php.h>

PHP_FUNCTION( mongo_bindata___construct );
PHP_FUNCTION( mongo_bindata___toString );
zval* bin_to_php_bin( char*, int, int );

PHP_FUNCTION( mongo_date___construct );
PHP_FUNCTION( mongo_date___toString );
zval* date_to_mongo_date( unsigned long long d );

PHP_FUNCTION( mongo_id___construct );
PHP_FUNCTION( mongo_id___toString );
zval* oid_to_mongo_id(const mongo::OID oid);

PHP_FUNCTION( mongo_regex___construct );
PHP_FUNCTION( mongo_regex___toString );
zval* re_to_mongo_re(char *regex, char *flags);

