/**
 *  Copyright 2013-2014 MongoDB, Inc.
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
#ifndef __API_BATCH_H__
#define __API_BATCH_H__ 1

void php_mongo_api_batch_make(mongo_write_batch_object *intern, char *dbname, char *collectionname, php_mongo_write_types type TSRMLS_DC);
void php_mongo_api_batch_make_from_collection_object(mongo_write_batch_object *intern, zval *zcollection, php_mongo_write_types type TSRMLS_DC);
void php_mongo_api_batch_free(php_mongo_batch *batch);
void php_mongo_api_batch_ctor(mongo_write_batch_object *intern, zval *zcollection, php_mongo_write_types type, HashTable *write_options TSRMLS_DC);
int php_mongo_api_batch_finalize(mongo_buffer *buf, int container_pos, int batch_pos, int max_bson_size, php_mongo_write_options *write_options TSRMLS_DC);
int php_mongo_api_batch_send_and_read(mongo_buffer *buf, int request_id, mongo_connection *connection, mongo_server_options *server_options, zval *return_value TSRMLS_DC);
int php_mongo_api_batch_execute(php_mongo_batch *batch, php_mongo_write_options *write_options, mongo_connection *connection, mongo_server_options *server_options, zval *return_value TSRMLS_DC);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
