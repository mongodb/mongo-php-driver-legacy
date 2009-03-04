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

#ifndef PHP_MONGO_H
#define PHP_MONGO_H 1

#define PHP_MONGO_VERSION "1.0"
#define PHP_MONGO_EXTNAME "mongo"

#define PHP_CONNECTION_RES_NAME "mongo connection"
#define PHP_AUTH_CONNECTION_RES_NAME "mongo authenticated connection"
#define PHP_DB_CURSOR_RES_NAME "mongo cursor"
#define PHP_GRIDFS_RES_NAME "gridfs tools"
#define PHP_GRIDFILE_RES_NAME "gridfs file"
#define PHP_GRIDFS_CHUNK_RES_NAME "gridfs file chunk"


PHP_MINIT_FUNCTION(mongo);

PHP_FUNCTION(mongo_connect);
PHP_FUNCTION(mongo_pconnect);
PHP_FUNCTION(mongo_close);
PHP_FUNCTION(mongo_query);
PHP_FUNCTION(mongo_find_one);
PHP_FUNCTION(mongo_remove);
PHP_FUNCTION(mongo_insert);
PHP_FUNCTION(mongo_batch_insert);
PHP_FUNCTION(mongo_update);

PHP_FUNCTION(mongo_has_next);
PHP_FUNCTION(mongo_next);

static void php_mongo_do_connect(INTERNAL_FUNCTION_PARAMETERS, int);

extern zend_module_entry mongo_module_entry;
#define phpext_mongo_ptr &mongo_module_entry

#endif
