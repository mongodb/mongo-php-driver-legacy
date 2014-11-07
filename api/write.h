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
} php_mongo_write_options;

typedef struct {
	zval *query;
	zval *update;
	int multi;
	int upsert;
} php_mongo_write_update_args;

typedef struct {
	zval *query;
	int limit;
} php_mongo_write_delete_args;

typedef enum {
	MONGODB_API_COMMAND_INSERT = 1,
	MONGODB_API_COMMAND_UPDATE = 2,
	MONGODB_API_COMMAND_DELETE = 3
} php_mongo_write_types;

typedef struct {
	php_mongo_write_types type;
	union {
		HashTable *insert_doc;
		php_mongo_write_update_args *update_args;
		php_mongo_write_delete_args *delete_args;
	} write;
} php_mongo_write_item;

typedef struct _php_mongo_batch {
	struct _php_mongo_batch *first;
	struct _php_mongo_batch *next;
	mongo_buffer buffer;
	int request_id;
	int container_pos;
	int batch_pos;
	int item_count;
} php_mongo_batch;

typedef struct {
	int     flags;
	int64_t cursor_id;
	int     start;
	int     returned;
} php_mongo_reply;

void php_mongo_api_write_options_from_zval(php_mongo_write_options *write_options, zval *z_write_options TSRMLS_DC);
void php_mongo_api_write_options_from_ht(php_mongo_write_options *write_options, HashTable *hindex TSRMLS_DC);
void php_mongo_api_write_options_to_zval(php_mongo_write_options *write_options, zval *z_write_options);

int php_mongo_api_write_header(mongo_buffer *buf, char *ns TSRMLS_DC);
int php_mongo_api_write_start(mongo_buffer *buf, php_mongo_write_types type, char *collection TSRMLS_DC);
int php_mongo_api_write_add(mongo_buffer *buf, int n, php_mongo_write_item *item, int max_document_size TSRMLS_DC);
int php_mongo_api_write_end(mongo_buffer *buf, int container_pos, int batch_pos, int max_write_size, php_mongo_write_options *write_options TSRMLS_DC);

int php_mongo_api_insert_single(mongo_buffer *buf, char *ns, char *collection, zval *doc, php_mongo_write_options *write_options, mongo_connection *connection TSRMLS_DC);
int php_mongo_api_update_single(mongo_buffer *buf, char *ns, char *collection, php_mongo_write_update_args *update_args, php_mongo_write_options *write_options, mongo_connection *connection TSRMLS_DC);
int php_mongo_api_delete_single(mongo_buffer *buf, char *ns, char *collection, php_mongo_write_delete_args *delete_args, php_mongo_write_options *write_options, mongo_connection *connection TSRMLS_DC);

int php_mongo_api_get_reply(mongo_con_manager *manager, mongo_connection *connection, mongo_server_options *options, int socket_read_timeout, int request_id, zval **retval TSRMLS_DC);
int php_mongo_api_raise_exception_on_write_failure(mongo_connection *connection, zval *document TSRMLS_DC);
int php_mongo_api_raise_exception_on_command_failure(mongo_connection *connection, zval *document TSRMLS_DC);

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
