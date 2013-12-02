/**
 *  Copyright 2013 MongoDB, Inc.
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
#ifndef __API_WRITE_H__
#define __API_WRITE_H__ 1


typedef struct {
	int        wtype; /* 0=use-default, 1=w, 2=wstring */
	union {
		int    w;
		char  *wstring;
	} write_concern;
	int        wtimeout;
	int        j;
	int        fsync;
	int        ordered;
} php_mongodb_write_options;

typedef struct {
	int     flags;
	int64_t cursor_id;
	int     start;
	int     returned;
} php_mongodb_reply;

void php_mongo_api_write_options_from_zval(php_mongodb_write_options *write_options, zval *z_write_options);
void php_mongo_api_write_options_to_zval(php_mongodb_write_options *write_options, zval *z_write_options);
int php_mongo_api_insert_single(mongo_buffer *buf, char *ns, char *collection, zval *doc, php_mongodb_write_options *write_options, mongo_connection *connection TSRMLS_DC);
int php_mongo_api_insert_start(mongo_buffer *buf, char *ns, char *collection, php_mongodb_write_options *write_options TSRMLS_DC);
int php_mongo_api_insert_add(mongo_buffer *buf, int n, HashTable *document, int max_document_size TSRMLS_DC);
int php_mongo_api_insert_end(mongo_buffer *buf, int container_pos, int max_write_size TSRMLS_DC);
int php_mongo_api_get_reply(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *options, int socket_read_timeout, int request_id, zval **retval TSRMLS_DC);

/* SERVER-10643: maxBsonWireObjectSize isn't exposed yet, so we need to cruft the
 * additional padding we are allowed ourself (used for document overhead+write options) */
#define WRITE_COMMAND_PADDING (16 * 1024)
#define MAX_BSON_WIRE_OBJECT_SIZE(size) (size+WRITE_COMMAND_PADDING)

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
