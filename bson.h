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
#ifndef PHP_BSON_H
#define PHP_BSON_H 1


#define OID_SIZE 12

/* BSON type constants from http://bsonspec.org/#/specification */
#define BSON_DOUBLE    0x01
#define BSON_STRING    0x02
#define BSON_OBJECT    0x03
#define BSON_ARRAY     0x04
#define BSON_BINARY    0x05
#define BSON_UNDEF     0x06
#define BSON_OID       0x07
#define BSON_BOOL      0x08
#define BSON_DATE      0x09
#define BSON_NULL      0x0A
#define BSON_REGEX     0x0B
#define BSON_DBPOINTER 0x0C
#define BSON_CODE__D   0x0D
#define BSON_SYMBOL    0x0E
#define BSON_CODE      0x0F
#define BSON_INT       0x10
#define BSON_TIMESTAMP 0x11
#define BSON_LONG      0x12
#define BSON_MINKEY    0xFF
#define BSON_MAXKEY    0x7F

#define GROW_SLOWLY 1048576

/* Options used for conversion between zval and bson and v.v. */
#define BSON_OPT_DONT_FORCE_LONG_AS_OBJECT  0x00
#define BSON_OPT_FORCE_LONG_AS_OBJECT       0x01
#define BSON_OPT_INT32_LONG_AS_OBJECT       0x02

#define CREATE_BUF_STATIC(n) char b[n];         \
	buf.start = buf.pos = b;                    \
	buf.end = b+n;

/* driver */
int php_mongo_serialize_element(const char* name, int name_len, zval**, mongo_buffer*, int TSRMLS_DC);

/* objects */
void php_mongo_serialize_date(mongo_buffer*, zval* TSRMLS_DC);
void php_mongo_serialize_regex(mongo_buffer*, zval* TSRMLS_DC);
void php_mongo_serialize_code(mongo_buffer*, zval* TSRMLS_DC);
void php_mongo_serialize_ts(mongo_buffer*, zval* TSRMLS_DC);
void php_mongo_serialize_bin_data(mongo_buffer*, zval* TSRMLS_DC);
void php_mongo_serialize_int32(mongo_buffer*, zval* TSRMLS_DC);
void php_mongo_serialize_int64(mongo_buffer*, zval* TSRMLS_DC);

/* simple types */
void php_mongo_serialize_double(mongo_buffer*, double);
void php_mongo_serialize_string(mongo_buffer*, const char*, int);
void php_mongo_serialize_long(mongo_buffer*, int64_t);
void php_mongo_serialize_int(mongo_buffer*, int);
void php_mongo_serialize_byte(mongo_buffer*, char);
void php_mongo_serialize_bytes(mongo_buffer*, const char*, int);
void php_mongo_serialize_key(mongo_buffer*, const char*, int, int TSRMLS_DC);
void php_mongo_serialize_ns(mongo_buffer*, const char* TSRMLS_DC);

/* End a complex type */
int php_mongo_serialize_size(char *start, const mongo_buffer *buf, int max_size TSRMLS_DC);

int php_mongo_write_insert(mongo_buffer*, const char*, zval*, int max_document_size, int max_message_size TSRMLS_DC);
int php_mongo_write_batch_insert(mongo_buffer*, const char*, int flags, zval*, int max_document_size, int max_message_size TSRMLS_DC);
int php_mongo_write_query(mongo_buffer*, mongo_cursor*, int max_document_size, int max_message_size TSRMLS_DC);
int php_mongo_write_get_more(mongo_buffer*, mongo_cursor* TSRMLS_DC);
int php_mongo_write_delete(mongo_buffer*, const char*, int, zval*, int max_document_size, int max_message_size TSRMLS_DC);
int php_mongo_write_update(mongo_buffer*, const char*, int, zval*, zval*, int max_document_size, int max_message_size TSRMLS_DC);
int php_mongo_write_kill_cursors(mongo_buffer*, int64_t, int max_message_size TSRMLS_DC);

#define php_mongo_set_type(buf, type) php_mongo_serialize_byte(buf, (char)type)
#define php_mongo_serialize_null(buf) php_mongo_serialize_byte(buf, (char)0)
#define php_mongo_serialize_bool(buf, b) php_mongo_serialize_byte(buf, (char)b)

int resize_buf(mongo_buffer*, int);

int zval_to_bson(mongo_buffer*, HashTable*, int, int max_document_size TSRMLS_DC);

typedef struct {
	int level;
	int flag_cmd_cursor_as_int64;
} mongo_bson_conversion_options;

#define MONGO_BSON_CONVERSION_OPTIONS_INIT { 0, 0 }

/* Converts a BSON document to a zval. The conversions options are a bitmask
 * of the BSON_OPT_* constants */
const char* bson_to_zval(const char *buf, size_t buf_len, HashTable *result, mongo_bson_conversion_options *options TSRMLS_DC);
const char* bson_to_zval_iter(const char *buf, size_t buf_len, HashTable *result, mongo_bson_conversion_options *options TSRMLS_DC);

/* Initialize buffer to contain "\0", so mongo_buf_append will start appending
 * at the beginning. */
void mongo_buf_init(char *dest);

/* Takes a buffer and a string to add to the buffer.  The buffer must be large
 * enough to append the string and the string must be null-terminated. This
 * will not work for strings containing null characters (e.g., BSON). */
void mongo_buf_append(char *dest, const char *piece);

void php_mongo_handle_int64(zval **value, int64_t nr, int force_as_object TSRMLS_DC);

#if PHP_C_BIGENDIAN
/* Reverse the bytes in an int, wheeee stupid byte tricks */
# define BYTE1_32(b) ((b & 0xff000000) >> 24)
# define BYTE2_32(b) ((b & 0x00ff0000) >> 8)
# define BYTE3_32(b) ((b & 0x0000ff00) << 8)
# define BYTE4_32(b) ((b & 0x000000ff) << 24)
# define MONGO_32(b) (BYTE4_32(b) | BYTE3_32(b) | BYTE2_32(b) | BYTE1_32(b))

# define BYTE1_64(b) ((b & 0xff00000000000000ll) >> 56)
# define BYTE2_64(b) ((b & 0x00ff000000000000ll) >> 40)
# define BYTE3_64(b) ((b & 0x0000ff0000000000ll) >> 24)
# define BYTE4_64(b) ((b & 0x000000ff00000000ll) >> 8)
# define BYTE5_64(b) ((b & 0x00000000ff000000ll) << 8)
# define BYTE6_64(b) ((b & 0x0000000000ff0000ll) << 24)
# define BYTE7_64(b) ((b & 0x000000000000ff00ll) << 40)
# define BYTE8_64(b) ((b & 0x00000000000000ffll) << 56)
# define MONGO_64(b) (BYTE8_64(b) | BYTE7_64(b) | BYTE6_64(b) | BYTE5_64(b) | BYTE4_64(b) | BYTE3_64(b) | BYTE2_64(b) | BYTE1_64(b))
#else
# define MONGO_32(b) (b)
# define MONGO_64(b) (b)
#endif

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
