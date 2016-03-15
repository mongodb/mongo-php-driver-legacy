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
#include <php.h>
#include <zend_exceptions.h>

#ifdef WIN32
#  include <memory.h>
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#endif

#include "php_mongo.h"
#include "bson.h"
#include "types/bin_data.h"
#include "types/date.h"
#include "types/id.h"
#include "cursor_shared.h"

#define CHECK_BUFFER_LEN(len) \
	do { \
		if (buf + (len) >= buf_end) { \
			zval_ptr_dtor(&value); \
			zend_throw_exception_ex(mongo_ce_CursorException, 21 TSRMLS_CC, "Reading data for type %02x would exceed buffer for key \"%s\"", (unsigned char) type, name); \
			return NULL; \
		} \
	} while (0)

extern zend_class_entry *mongo_ce_BinData,
	*mongo_ce_Code,
	*mongo_ce_Date,
	*mongo_ce_Id,
	*mongo_ce_Regex,
	*mongo_ce_Timestamp,
	*mongo_ce_MinKey,
	*mongo_ce_MaxKey,
	*mongo_ce_Exception,
	*mongo_ce_CursorException,
	*mongo_ce_Int32,
	*mongo_ce_Int64;

ZEND_EXTERN_MODULE_GLOBALS(mongo)

static int prep_obj_for_db(mongo_buffer *buf, HashTable *array TSRMLS_DC);
static int apply_func_args_wrapper(void **data TSRMLS_DC, int num_args, va_list args, zend_hash_key *key);
static int is_utf8(const char *s, int len);
static int insert_helper(mongo_buffer *buf, zval *doc, int max TSRMLS_DC);

/* The position is not increased, we are just filling in the first 4 bytes with
 * the size.  */
int php_mongo_serialize_size(char *start, const mongo_buffer *buf, int max_size TSRMLS_DC)
{
	int total = MONGO_32((buf->pos - start));

	if (buf->pos - start > max_size) {
		zend_throw_exception_ex(mongo_ce_Exception, 3 TSRMLS_CC, "document fragment is too large: %d, max: %d", buf->pos - start, max_size);
		return FAILURE;
	}
	memcpy(start, &total, INT_32);
	return SUCCESS;
}


static int prep_obj_for_db(mongo_buffer *buf, HashTable *array TSRMLS_DC)
{
	zval **data, *newid;

	/* if _id field doesn't exist, add it */
	if (zend_hash_find(array, "_id", 4, (void**)&data) == FAILURE) {
		/* create new MongoId */
		MAKE_STD_ZVAL(newid);
		object_init_ex(newid, mongo_ce_Id);
		php_mongo_mongoid_populate(newid, NULL TSRMLS_CC);

		/* add to obj */
		zend_hash_add(array, "_id", 4, &newid, sizeof(zval*), NULL);

		/* set to data */
		data = &newid;
	}

	php_mongo_serialize_element("_id", 3, data, buf, 0 TSRMLS_CC);
	if (EG(exception)) {
		return FAILURE;
	}

	return SUCCESS;
}


/* serialize a zval */
int zval_to_bson(mongo_buffer *buf, HashTable *hash, int prep, int max_document_size TSRMLS_DC)
{
	uint start;
	int num = 0;

	/* check buf size */
	if (BUF_REMAINING <= 5) {
		resize_buf(buf, 5);
	}

	/* keep a record of the starting position as an offset, in case the memory
	 * is resized */
	start = buf->pos-buf->start;

	/* skip first 4 bytes to leave room for size */
	buf->pos += INT_32;

	/* It doesn't matter if the document is empty if we need to prep it */
	if (zend_hash_num_elements(hash) > 0 || prep) {
		if (prep) {
			prep_obj_for_db(buf, hash TSRMLS_CC);
			num++;
		}

		zend_hash_apply_with_arguments(hash TSRMLS_CC, (apply_func_args_t)apply_func_args_wrapper, 3, buf, prep, &num);
	}

	php_mongo_serialize_null(buf);
	php_mongo_serialize_size(buf->start + start, buf, max_document_size TSRMLS_CC);
	return EG(exception) ? FAILURE : num;
}

static int apply_func_args_wrapper(void **data TSRMLS_DC, int num_args, va_list args, zend_hash_key *key)
{
	mongo_buffer *buf = va_arg(args, mongo_buffer*);
	int prep = va_arg(args, int);
	int *num = va_arg(args, int*);

	if (key->nKeyLength) {
		return php_mongo_serialize_element(key->arKey, key->nKeyLength - 1, (zval**)data, buf, prep TSRMLS_CC);
	} else {
		long current = key->h;
		int pos = 29, negative = 0;
		char name[30];

		/* If the key is a number in ascending order, we're still dealing with
		 * an array, not an object, so increase the count */
		if (key->h == (unsigned int)*num) {
			(*num)++;
		}

		name[pos--] = '\0';

		/* take care of negative indexes */
		if (current < 0) {
			current *= -1;
			negative = 1;
		}

		do {
			int digit = current % 10;

			digit += (int)'0';
			name[pos--] = (char)digit;
			current = current / 10;
		} while (current > 0);

		if (negative) {
			name[pos--] = '-';
		}

		return php_mongo_serialize_element(name + pos + 1, strlen(name + pos + 1), (zval**)data, buf, prep TSRMLS_CC);
	}
}

int php_mongo_serialize_element(const char *name, int name_len, zval **data, mongo_buffer *buf, int prep TSRMLS_DC)
{
	if (prep && strcmp(name, "_id") == 0) {
		return ZEND_HASH_APPLY_KEEP;
	}

	switch (Z_TYPE_PP(data)) {
		case IS_NULL:
			PHP_MONGO_SERIALIZE_KEY(BSON_NULL);
			break;

		case IS_LONG:
			if (MonGlo(native_long)) {
#if SIZEOF_LONG == 4
			PHP_MONGO_SERIALIZE_KEY(BSON_INT);
			php_mongo_serialize_int(buf, Z_LVAL_PP(data));
#else
# if SIZEOF_LONG == 8
			PHP_MONGO_SERIALIZE_KEY(BSON_LONG);
			php_mongo_serialize_long(buf, Z_LVAL_PP(data));
# else
#  error The PHP number size is neither 4 or 8 bytes; no clue what to do with that!
# endif
#endif
			} else {
				PHP_MONGO_SERIALIZE_KEY(BSON_INT);
				php_mongo_serialize_int(buf, Z_LVAL_PP(data));
			}
			break;

		case IS_DOUBLE:
			PHP_MONGO_SERIALIZE_KEY(BSON_DOUBLE);
			php_mongo_serialize_double(buf, Z_DVAL_PP(data));
			break;

		case IS_BOOL:
			PHP_MONGO_SERIALIZE_KEY(BSON_BOOL);
			php_mongo_serialize_bool(buf, Z_BVAL_PP(data));
			break;

		case IS_STRING: {
			PHP_MONGO_SERIALIZE_KEY(BSON_STRING);

			/* if this is not a valid string, stop */
			if (!is_utf8(Z_STRVAL_PP(data), Z_STRLEN_PP(data))) {
				zend_throw_exception_ex(mongo_ce_Exception, 12 TSRMLS_CC, "non-utf8 string: %s", Z_STRVAL_PP(data));
				return ZEND_HASH_APPLY_STOP;
			}

			php_mongo_serialize_int(buf, Z_STRLEN_PP(data) + 1);
			php_mongo_serialize_string(buf, Z_STRVAL_PP(data), Z_STRLEN_PP(data));
			break;
		}

		case IS_ARRAY: {
			int num;
			/* if we realloc, we need an offset, not an abs pos (phew) */
			int type_offset = buf->pos-buf->start;

			/* serialize */
			PHP_MONGO_SERIALIZE_KEY(BSON_ARRAY);
			num = zval_to_bson(buf, Z_ARRVAL_PP(data), NO_PREP, MONGO_DEFAULT_MAX_DOCUMENT_SIZE TSRMLS_CC);
			if (EG(exception)) {
				return ZEND_HASH_APPLY_STOP;
			}

			/* now go back and set the type bit */
			if (num == zend_hash_num_elements(Z_ARRVAL_PP(data))) {
				buf->start[type_offset] = BSON_ARRAY;
			} else {
				buf->start[type_offset] = BSON_OBJECT;
			}

			break;
		}

		case IS_OBJECT: {
			zend_class_entry *clazz = Z_OBJCE_PP(data);

			/* check for defined classes */
			/* MongoId */
			if (clazz == mongo_ce_Id) {
				mongo_id *id;

				PHP_MONGO_SERIALIZE_KEY(BSON_OID);
				id = (mongo_id*)zend_object_store_get_object(*data TSRMLS_CC);
				if (!id->id) {
					return ZEND_HASH_APPLY_KEEP;
				}

				php_mongo_serialize_bytes(buf, id->id, OID_SIZE);
			}
			/* MongoDate */
			else if (clazz == mongo_ce_Date) {
				PHP_MONGO_SERIALIZE_KEY(BSON_DATE);
				php_mongo_serialize_date(buf, *data TSRMLS_CC);
			}
			/* MongoRegex */
			else if (clazz == mongo_ce_Regex) {
				PHP_MONGO_SERIALIZE_KEY(BSON_REGEX);
				php_mongo_serialize_regex(buf, *data TSRMLS_CC);
			}
			/* MongoCode */
			else if (clazz == mongo_ce_Code) {
				PHP_MONGO_SERIALIZE_KEY(BSON_CODE);
				php_mongo_serialize_code(buf, *data TSRMLS_CC);
				if (EG(exception)) {
					return ZEND_HASH_APPLY_STOP;
				}
			}
			/* MongoBin */
			else if (clazz == mongo_ce_BinData) {
				PHP_MONGO_SERIALIZE_KEY(BSON_BINARY);
				php_mongo_serialize_bin_data(buf, *data TSRMLS_CC);
			}
			/* MongoTimestamp */
			else if (clazz == mongo_ce_Timestamp) {
				PHP_MONGO_SERIALIZE_KEY(BSON_TIMESTAMP);
				php_mongo_serialize_ts(buf, *data TSRMLS_CC);
			}
			/* MongoMinKey */
			else if (clazz == mongo_ce_MinKey) {
				PHP_MONGO_SERIALIZE_KEY(BSON_MINKEY);
			}
			/* MongoMaxKey */
			else if (clazz == mongo_ce_MaxKey) {
				PHP_MONGO_SERIALIZE_KEY(BSON_MAXKEY);
			}
			/* Integer types */
			else if (clazz == mongo_ce_Int32) {
				PHP_MONGO_SERIALIZE_KEY(BSON_INT);
				php_mongo_serialize_int32(buf, *data TSRMLS_CC);
			}
			else if (clazz == mongo_ce_Int64) {
				PHP_MONGO_SERIALIZE_KEY(BSON_LONG);
				php_mongo_serialize_int64(buf, *data TSRMLS_CC);
			}
			/* serialize a normal object */
			else {
				HashTable *hash = Z_OBJPROP_PP(data);

				/* go through the k/v pairs and serialize them */
				PHP_MONGO_SERIALIZE_KEY(BSON_OBJECT);

				zval_to_bson(buf, hash, NO_PREP, MONGO_DEFAULT_MAX_MESSAGE_SIZE TSRMLS_CC);
				if (EG(exception)) {
					return ZEND_HASH_APPLY_STOP;
				}
			}
			break;
		}
	}

	return ZEND_HASH_APPLY_KEEP;
}

int resize_buf(mongo_buffer *buf, int size)
{
	int total = buf->end - buf->start;
	int used = buf->pos - buf->start;

	total = total < GROW_SLOWLY ? total*2 : total + INITIAL_BUF_SIZE;
	while (total-used < size) {
		total += size;
	}

	buf->start = (char*)erealloc(buf->start, total);
	buf->pos = buf->start + used;
	buf->end = buf->start + total;
	return total;
}

/*
 * create a bson date
 *
 * type: 9
 * 8 bytes of ms since the epoch
 */
void php_mongo_serialize_date(mongo_buffer *buf, zval *date TSRMLS_DC)
{
	int64_t ms;
	zval *sec = zend_read_property(mongo_ce_Date, date, "sec", 3, 0 TSRMLS_CC);
	zval *usec = zend_read_property(mongo_ce_Date, date, "usec", 4, 0 TSRMLS_CC);

	ms = ((int64_t)Z_LVAL_P(sec) * 1000) + ((int64_t)Z_LVAL_P(usec) / 1000);
	php_mongo_serialize_long(buf, ms);
}


/*
 * create a bson int from an Int32 object
 */
void php_mongo_serialize_int32(mongo_buffer *buf, zval *data TSRMLS_DC)
{
	int value;
	zval *zvalue = zend_read_property(mongo_ce_Int32, data, "value", 5, 0 TSRMLS_CC);
	value = strtol(Z_STRVAL_P(zvalue), NULL, 10);

	php_mongo_serialize_int(buf, value);
}

/*
 * create a bson long from an Int64 object
 */
void php_mongo_serialize_int64(mongo_buffer *buf, zval *data TSRMLS_DC)
{
	int64_t value;
	zval *zvalue = zend_read_property(mongo_ce_Int64, data, "value", 5, 0 TSRMLS_CC);
	value = strtoll(Z_STRVAL_P(zvalue), NULL, 10);

	php_mongo_serialize_long(buf, value);
}

/*
 * create a bson regex
 *
 * type: 11
 * cstring cstring
 */
void php_mongo_serialize_regex(mongo_buffer *buf, zval *regex TSRMLS_DC)
{
	zval *z;

	z = zend_read_property(mongo_ce_Regex, regex, "regex", 5, 0 TSRMLS_CC);
	php_mongo_serialize_string(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
	z = zend_read_property(mongo_ce_Regex, regex, "flags", 5, 0 TSRMLS_CC);
	php_mongo_serialize_string(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
}

/*
 * create a bson code with scope
 *
 * type: 15
 * 4 bytes total size
 * 4 bytes cstring size + NULL
 * cstring
 * bson object scope
 */
void php_mongo_serialize_code(mongo_buffer *buf, zval *code TSRMLS_DC)
{
	uint start;
	zval *zid;

	/* save spot for size */
	start = buf->pos-buf->start;
	buf->pos += INT_32;
	zid = zend_read_property(mongo_ce_Code, code, "code", 4, NOISY TSRMLS_CC);
	/* string size */
	php_mongo_serialize_int(buf, Z_STRLEN_P(zid) + 1);
	/* string */
	php_mongo_serialize_string(buf, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
	/* scope */
	zid = zend_read_property(mongo_ce_Code, code, "scope", 5, NOISY TSRMLS_CC);
	zval_to_bson(buf, HASH_P(zid), NO_PREP, MONGO_DEFAULT_MAX_MESSAGE_SIZE TSRMLS_CC);
	if (EG(exception)) {
		return;
	}

	/* get total size */
	php_mongo_serialize_size(buf->start + start, buf, MONGO_DEFAULT_MAX_MESSAGE_SIZE TSRMLS_CC);
}

/*
 * create bson binary data
 *
 * type: 5
 * 4 bytes: length of bindata
 * 1 byte: bindata type
 * bindata
 */
void php_mongo_serialize_bin_data(mongo_buffer *buf, zval *bin TSRMLS_DC)
{
	zval *zbin, *ztype;

	zbin = zend_read_property(mongo_ce_BinData, bin, "bin", 3, 0 TSRMLS_CC);
	ztype = zend_read_property(mongo_ce_BinData, bin, "type", 4, 0 TSRMLS_CC);

	if (
		Z_LVAL_P(ztype) == PHP_MONGO_BIN_UUID_RFC4122 &&
		Z_STRLEN_P(zbin) != PHP_MONGO_BIN_UUID_RFC4122_SIZE
	) {
		zend_throw_exception_ex(mongo_ce_Exception, 25 TSRMLS_CC, "RFC4122 UUID must be %d bytes; actually: %d", PHP_MONGO_BIN_UUID_RFC4122_SIZE, Z_STRLEN_P(zbin));
		return;
	}

	/*
	 * type 2 has the redundant structure:
	 *
	 * |------|--|-------==========|
	 *  length 02 length   bindata
	 *
	 *   - 4 bytes: length of bindata (+4 for length below)
	 *   - 1 byte type (0x02)
	 *   - N bytes: 4 bytes of length of the following bindata + bindata
	 *
	 * All other types have:
	 *
	 * |------|--|==========|
	 *  length     bindata
	 *        type
	 */
	if (Z_LVAL_P(ztype) == PHP_MONGO_BIN_BYTE_ARRAY) {
		/* length */
		php_mongo_serialize_int(buf, Z_STRLEN_P(zbin) + 4);
		/* 02 */
		php_mongo_serialize_byte(buf, PHP_MONGO_BIN_BYTE_ARRAY);
		/* length */
		php_mongo_serialize_int(buf, Z_STRLEN_P(zbin));
	} else {
		/* length */
		php_mongo_serialize_int(buf, Z_STRLEN_P(zbin));
		/* type */
		php_mongo_serialize_byte(buf, (unsigned char)Z_LVAL_P(ztype));
	}

	/* bindata */
	php_mongo_serialize_bytes(buf, Z_STRVAL_P(zbin), Z_STRLEN_P(zbin));
}

/*
 * create bson timestamp
 *
 * type: 17
 * 4 bytes seconds since epoch
 * 4 bytes increment
 */
void php_mongo_serialize_ts(mongo_buffer *buf, zval *time TSRMLS_DC)
{
	zval *ts, *inc;

	ts = zend_read_property(mongo_ce_Timestamp, time, "sec", strlen("sec"), NOISY TSRMLS_CC);
	inc = zend_read_property(mongo_ce_Timestamp, time, "inc", strlen("inc"), NOISY TSRMLS_CC);

	php_mongo_serialize_int(buf, Z_LVAL_P(inc));
	php_mongo_serialize_int(buf, Z_LVAL_P(ts));
}

void php_mongo_serialize_byte(mongo_buffer *buf, char b)
{
	if (BUF_REMAINING <= 1) {
		resize_buf(buf, 1);
	}
	*(buf->pos) = b;
	buf->pos += 1;
}

void php_mongo_serialize_bytes(mongo_buffer *buf, const char *str, int str_len)
{
	if (BUF_REMAINING <= str_len) {
		resize_buf(buf, str_len);
	}
	memcpy(buf->pos, str, str_len);
	buf->pos += str_len;
}

void php_mongo_serialize_string(mongo_buffer *buf, const char *str, int str_len)
{
	if (BUF_REMAINING <= str_len + 1) {
		resize_buf(buf, str_len + 1);
	}

	memcpy(buf->pos, str, str_len);
	/* add \0 at the end of the string */
	buf->pos[str_len] = 0;
	buf->pos += str_len + 1;
}

void php_mongo_serialize_int(mongo_buffer *buf, int num)
{
	int i = MONGO_32(num);

	if (BUF_REMAINING <= INT_32) {
		resize_buf(buf, INT_32);
	}

	memcpy(buf->pos, &i, INT_32);
	buf->pos += INT_32;
}

void php_mongo_serialize_long(mongo_buffer *buf, int64_t num)
{
	int64_t i = MONGO_64(num);

	if (BUF_REMAINING <= INT_64) {
		resize_buf(buf, INT_64);
	}

	memcpy(buf->pos, &i, INT_64);
	buf->pos += INT_64;
}

void php_mongo_serialize_double(mongo_buffer *buf, double num)
{
	int64_t dest, *dest_p;

	dest_p = &dest;
	memcpy(dest_p, &num, 8);
	dest = MONGO_64(dest);

	if (BUF_REMAINING <= INT_64) {
		resize_buf(buf, INT_64);
	}

	memcpy(buf->pos, dest_p, DOUBLE_64);
	buf->pos += DOUBLE_64;
}

/*
 * prep == true
 * we are inserting, so keys can't have .'s in them
 */
void php_mongo_serialize_key(mongo_buffer *buf, const char *str, int str_len, int prep TSRMLS_DC)
{
	if (str[0] == '\0' && !MonGlo(allow_empty_keys)) {
		zend_throw_exception_ex(mongo_ce_Exception, 1 TSRMLS_CC, "zero-length keys are not allowed, did you use $ with double quotes?");
		return;
	}

	if (BUF_REMAINING <= str_len + 1) {
		resize_buf(buf, str_len + 1);
	}

	if (memchr(str, '\0', str_len) != NULL) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "'\\0' not allowed in key: %s\\0...", str);
		return;
	}

	if (prep && (strchr(str, '.') != 0)) {
		zend_throw_exception_ex(mongo_ce_Exception, 2 TSRMLS_CC, "'.' not allowed in key: %s", str);
		return;
	}

	if (MonGlo(cmd_char) && strchr(str, MonGlo(cmd_char)[0]) == str) {
		*(buf->pos) = '$';
		memcpy(buf->pos + 1, str + 1, str_len-1);
	} else {
		memcpy(buf->pos, str, str_len);
	}

	/* add \0 at the end of the string */
	buf->pos[str_len] = 0;
	buf->pos += str_len + 1;
}

/*
 * replaces collection names starting with MonGlo(cmd_char)
 * with the '$' character.
 *
 * TODO: this doesn't handle main.$oplog-type situations (if
 * MonGlo(cmd_char) is set)
 */
void php_mongo_serialize_ns(mongo_buffer *buf, const char *str TSRMLS_DC)
{
	char *collection = strchr(str, '.') + 1;

	if (BUF_REMAINING <= (int)strlen(str) + 1) {
		resize_buf(buf, strlen(str) + 1);
	}

	if (MonGlo(cmd_char) && strchr(collection, MonGlo(cmd_char)[0]) == collection) {
		memcpy(buf->pos, str, collection-str);
		buf->pos += collection-str;
		*(buf->pos) = '$';
		memcpy(buf->pos + 1, collection + 1, strlen(collection)-1);
		buf->pos[strlen(collection)] = 0;
		buf->pos += strlen(collection) + 1;
	} else {
		memcpy(buf->pos, str, strlen(str));
		buf->pos[strlen(str)] = 0;
		buf->pos += strlen(str) + 1;
	}
}

/* Returns:
 *  0 on success,
 * -1 when an exception in zval_to_bson was thrown
 * -3 when a fragment or document was too large
 * An exception is also thrown when the return value is not 0 */
static int insert_helper(mongo_buffer *buf, zval *doc, int max_document_size TSRMLS_DC)
{
	int start = buf->pos - buf->start;
	int result = zval_to_bson(buf, HASH_P(doc), PREP, max_document_size TSRMLS_CC);

	/* throw exception if serialization crapped out */
	if (EG(exception) || FAILURE == result) {
		return -1;
	}

	/* throw an exception if the doc was too big */
	if (buf->pos - (buf->start + start) > max_document_size) {
		zend_throw_exception_ex(mongo_ce_Exception, 5 TSRMLS_CC, "size of BSON doc is %d bytes, max is %d", buf->pos - (buf->start + start), max_document_size);
		return -3;
	}

	return (php_mongo_serialize_size(buf->start + start, buf, max_document_size TSRMLS_CC) == SUCCESS) ? 0 : -3;
}

int php_mongo_write_insert(mongo_buffer *buf, const char *ns, zval *doc, int max_document_size, int max_message_size TSRMLS_DC)
{
	mongo_msg_header header;
	int start = buf->pos - buf->start;

	CREATE_HEADER(buf, ns, OP_INSERT);

	if (insert_helper(buf, doc, max_document_size TSRMLS_CC) != 0) {
		return FAILURE;
	}

	return php_mongo_serialize_size(buf->start + start, buf, max_message_size TSRMLS_CC);
}

int php_mongo_write_batch_insert(mongo_buffer *buf, const char *ns, int flags, zval *docs, int max_document_size, int max_message_size TSRMLS_DC)
{
	int start = buf->pos - buf->start, count = 0;
	HashPosition pointer;
	zval **doc;
	mongo_msg_header header;

	CREATE_HEADER_WITH_OPTS(buf, ns, OP_INSERT, flags);

	for (
		zend_hash_internal_pointer_reset_ex(HASH_P(docs), &pointer);
		zend_hash_get_current_data_ex(HASH_P(docs), (void**)&doc, &pointer) == SUCCESS;
		zend_hash_move_forward_ex(HASH_P(docs), &pointer)
	) {
		if (IS_SCALAR_PP(doc)) {
			continue;
		}

		if (insert_helper(buf, *doc, max_document_size TSRMLS_CC) != 0) {
			/* An exception has already been thrown */
			return FAILURE;
		}

		if (buf->pos - buf->start >= max_message_size) {
			zend_throw_exception_ex(mongo_ce_Exception, 5 TSRMLS_CC, "current batch size is too large: %d, max: %d", buf->pos - buf->start, max_message_size);
			return FAILURE;
		}

		count++;
	}

	/* this is a hard limit in the db server (util/messages.cpp) */
	if (buf->pos - (buf->start + start) > max_message_size) {
		zend_throw_exception_ex(mongo_ce_Exception, 3 TSRMLS_CC, "insert too large: %d, max: %d", buf->pos - (buf->start + start), max_message_size);
		return FAILURE;
	}

	if (php_mongo_serialize_size(buf->start + start, buf, max_message_size TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}
	return count;
}

int php_mongo_write_update(mongo_buffer *buf, const char *ns, int flags, zval *criteria, zval *newobj, int max_document_size, int max_message_size TSRMLS_DC)
{
	mongo_msg_header header;
	int start = buf->pos - buf->start;

	CREATE_HEADER(buf, ns, OP_UPDATE);

	php_mongo_serialize_int(buf, flags);

	if (
		zval_to_bson(buf, HASH_P(criteria), NO_PREP, max_document_size TSRMLS_CC) == FAILURE ||
		EG(exception) ||
		zval_to_bson(buf, HASH_P(newobj), NO_PREP, max_document_size TSRMLS_CC) == FAILURE ||
		EG(exception) /* Having this twice does make sense, as zval_to_bson can thrown an exception */
	) {
		return FAILURE;
	}

	return php_mongo_serialize_size(buf->start + start, buf, max_message_size TSRMLS_CC);
}

int php_mongo_write_delete(mongo_buffer *buf, const char *ns, int flags, zval *criteria, int max_document_size, int max_message_size TSRMLS_DC)
{
	mongo_msg_header header;
	int start = buf->pos - buf->start;

	CREATE_HEADER(buf, ns, OP_DELETE);

	php_mongo_serialize_int(buf, flags);

	if (zval_to_bson(buf, HASH_P(criteria), NO_PREP, max_document_size TSRMLS_CC) == FAILURE || EG(exception)) {
		return FAILURE;
	}

	return php_mongo_serialize_size(buf->start + start, buf, max_message_size TSRMLS_CC);
}

/*
 * Creates a query string in buf.
 *
 * The following fields of cursor are used:
 *  - ns
 *  - opts
 *  - skip
 *  - limit
 *  - query
 *  - fields
 *
 */
int php_mongo_write_query(mongo_buffer *buf, mongo_cursor *cursor, int max_document_size, int max_message_size TSRMLS_DC)
{
	mongo_msg_header header;
	int start = buf->pos - buf->start;

	CREATE_HEADER_WITH_OPTS(buf, cursor->ns, OP_QUERY, cursor->opts);
	cursor->send.request_id = header.request_id;

	php_mongo_serialize_int(buf, cursor->skip);
	php_mongo_serialize_int(buf, php_mongo_calculate_next_request_limit(cursor));

	if (zval_to_bson(buf, HASH_P(cursor->query), NO_PREP, max_document_size TSRMLS_CC) == FAILURE || EG(exception)) {
		return FAILURE;
	}
	if (cursor->fields && zend_hash_num_elements(HASH_P(cursor->fields)) > 0) {
		if (zval_to_bson(buf, HASH_P(cursor->fields), NO_PREP, max_document_size TSRMLS_CC) == FAILURE || EG(exception)) {
			return FAILURE;
		}
	}

	return php_mongo_serialize_size(buf->start + start, buf, max_message_size TSRMLS_CC);
}

int php_mongo_write_kill_cursors(mongo_buffer *buf, int64_t cursor_id, int max_message_size TSRMLS_DC)
{
	mongo_msg_header header;

	CREATE_MSG_HEADER(MonGlo(request_id)++, 0, OP_KILL_CURSORS);
	APPEND_HEADER(buf, 0);

	/* # of cursors */
	php_mongo_serialize_int(buf, 1);
	/* cursor ids */
	php_mongo_serialize_long(buf, cursor_id);
	return php_mongo_serialize_size(buf->start, buf, max_message_size TSRMLS_CC);
}

/*
 * Creates a GET_MORE request
 *
 * The following fields of cursor are used:
 *  - ns
 *  - recv.request_id
 *  - limit
 *  - cursor_id
 */
int php_mongo_write_get_more(mongo_buffer *buf, mongo_cursor *cursor TSRMLS_DC)
{
	mongo_msg_header header;
	int start = buf->pos - buf->start;

	CREATE_RESPONSE_HEADER(buf, cursor->ns, cursor->recv.request_id, OP_GET_MORE);
	cursor->send.request_id = header.request_id;

	php_mongo_serialize_int(buf, php_mongo_calculate_next_request_limit(cursor));
	php_mongo_serialize_long(buf, cursor->cursor_id);

	return php_mongo_serialize_size(buf->start + start, buf, cursor->connection->max_message_size TSRMLS_CC);
}


/* Parses a single document from the BSON buffer and throws an exception if the
 * the buffer is not exhausted (in addition to errors from bson_to_zval_iter).
 */
const char* bson_to_zval(const char *buf, size_t buf_len, HashTable *result, mongo_bson_conversion_options *options TSRMLS_DC)
{
	const char *buf_end;

	buf_end = bson_to_zval_iter(buf, buf_len, result, options TSRMLS_CC);

	if (EG(exception)) {
		return NULL;
	}

	if (buf + buf_len != buf_end) {
		zend_throw_exception_ex(mongo_ce_CursorException, 42 TSRMLS_CC, "Document length (%u bytes) is not equal to buffer (%u bytes)", buf_end - buf, buf_len);
		return NULL;
	}

	return buf_end;
}


/* Parses a single document from the BSON buffer and returns a pointer to the
 * buffer position immediately following the document. If the buffer is invalid,
 * an exception will be thrown and NULL will be returned.
 */
const char* bson_to_zval_iter(const char *buf, size_t buf_len, HashTable *result, mongo_bson_conversion_options *options TSRMLS_DC)
{
	/* buf_start is used for debugging
	 *
	 * If the deserializer runs into bson it can't parse, it will dump the
	 * bytes to that point.
	 *
	 * We lose buf's position as we iterate, so we need buf_start to save it. */
	const char *buf_start = buf, *buf_end;
	unsigned char type;
	int doc_len;

	if (buf == 0) {
		return NULL;
	}

	if (buf_len < 5) {
		zend_throw_exception_ex(mongo_ce_CursorException, 38 TSRMLS_CC, "Reading document length would exceed buffer (%u bytes)", buf_len);
		return NULL;
	}

	doc_len = MONGO_32(*((int32_t*)buf));

	if (doc_len < 5) {
		zend_throw_exception_ex(mongo_ce_CursorException, 39 TSRMLS_CC, "Document length (%d bytes) should be at least 5 (i.e. empty document)", doc_len);
		return NULL;
	}

	if (buf_len < (size_t) doc_len) {
		zend_throw_exception_ex(mongo_ce_CursorException, 40 TSRMLS_CC, "Document length (%d bytes) exceeds buffer (%u bytes)", doc_len, buf_len);
		return NULL;
	}

	buf_end = buf + doc_len;

	/* for size */
	buf += INT_32;

	while ((type = *buf++) != 0) {
		const char *name;
		size_t name_len;
		zval *value;

		name = buf;
		name_len = strlen(name);

		if (buf + name_len + 1 >= buf_end) {
			zend_throw_exception_ex(mongo_ce_CursorException, 21 TSRMLS_CC, "Reading key name for type %02x would exceed buffer", (unsigned char) type);
			return NULL;
		}

		/* get past field name */
		buf += name_len + 1;

		MAKE_STD_ZVAL(value);
		ZVAL_NULL(value);

		/* get value */
		switch (type) {
			case BSON_OID: {
				mongo_id *this_id;
				const char *tmp_id;
				zval *str = 0;

				CHECK_BUFFER_LEN(OID_SIZE);

				object_init_ex(value, mongo_ce_Id);

				this_id = (mongo_id*)zend_object_store_get_object(value TSRMLS_CC);
				this_id->id = estrndup(buf, OID_SIZE);

				MAKE_STD_ZVAL(str);

				tmp_id = php_mongo_mongoid_to_hex(this_id->id);
				ZVAL_STRING(str, tmp_id, 0);
				zend_update_property(mongo_ce_Id, value, "$id", strlen("$id"), str TSRMLS_CC);
				zval_ptr_dtor(&str);

				buf += OID_SIZE;
				break;
			}

			case BSON_DOUBLE: {
				double d;
				int64_t i, *i_p;

				CHECK_BUFFER_LEN(DOUBLE_64);

				d = *(double*)buf;
				i_p = &i;

				memcpy(i_p, &d, DOUBLE_64);
				i = MONGO_64(i);
				memcpy(&d, i_p, DOUBLE_64);

				ZVAL_DOUBLE(value, d);
				buf += DOUBLE_64;
				break;
			}

			case BSON_SYMBOL:
			case BSON_STRING: {
				int len;

				CHECK_BUFFER_LEN(INT_32);

				len = MONGO_32(*((int32_t*)buf));
				buf += INT_32;

				/* len includes \0 */
				if (len < 1) {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 21 TSRMLS_CC, "invalid string length for key \"%s\": %d", name, len);
					return NULL;
				}

				CHECK_BUFFER_LEN(len);

				/* ensure that string is null-terminated */
				if (buf[len - 1] != '\0') {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 41 TSRMLS_CC, "string for key \"%s\" is not null-terminated", name);
					return NULL;
				}

				ZVAL_STRINGL(value, buf, len-1, 1);
				buf += len;
				break;
			}

			case BSON_OBJECT:
			case BSON_ARRAY: {
				int embedded_doc_len;

				/* Peek at the embedded document's length before recursing */
				CHECK_BUFFER_LEN(INT_32);
				embedded_doc_len = MONGO_32(*(int32_t*)buf);

				CHECK_BUFFER_LEN(embedded_doc_len);

				array_init(value);

				/* These two if statements make sure that the cmd_cursor_as_int64 flag only
				 * survives if previous conditions are also met. The check on level 0 is for
				 * command cursors (the "cursor" check) and for parallelCollectionScan (the
				 * "cursors" check). The check on level 2 is again only for
				 * parallelCollectionScan. The _id values as returned for command cursors and
				 * parallelCollectionScan have to be a MongoInt64() due to 32-bit platform
				 * wonkyness */
				if (options && options->flag_cmd_cursor_as_int64 && options->level == 0 && strcmp(name, "cursor") != 0 && strcmp(name, "cursors") != 0) {
					options->flag_cmd_cursor_as_int64 = 0;
				}
				if (options && options->flag_cmd_cursor_as_int64 && options->level == 2 && strcmp(name, "cursor") != 0) {
					options->flag_cmd_cursor_as_int64 = 0;
				}

				if (options) {
					options->level++;
				}
				buf = bson_to_zval(buf, embedded_doc_len, Z_ARRVAL_P(value), options TSRMLS_CC);
				if (options) {
					options->level--;
				}

				if (EG(exception)) {
					zval_ptr_dtor(&value);
					return NULL;
				}
				break;
			}

			case BSON_BINARY: {
				unsigned char subtype;
				int len;

				CHECK_BUFFER_LEN(INT_32);

				len = MONGO_32(*(int32_t*)buf);
				buf += INT_32;

				CHECK_BUFFER_LEN(BYTE_8);

				subtype = *buf++;

				/* If the subtype is 2, check if the binary data is prefixed by
				 * its length.
				 *
				 * There is an infinitesimally small chance that the first four
				 * bytes will happen to be the length of the rest of the
				 * string.  In this case, the data will be corrupted. */
				if ((int)subtype == 2) {
					int len2;

					CHECK_BUFFER_LEN(INT_32);

					len2 = MONGO_32(*(int32_t*)buf);

					/* If the lengths match, the data is to spec, so we use
					 * len2 as the true length. */
					if (len2 == len - 4) {
						len = len2;
						buf += INT_32;
					}
				}

				if (len < 0) {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 22 TSRMLS_CC, "invalid binary length for key \"%s\": %d", name, len);
					return NULL;
				}

				CHECK_BUFFER_LEN(len);

				if (subtype == PHP_MONGO_BIN_UUID_RFC4122 && len != PHP_MONGO_BIN_UUID_RFC4122_SIZE) {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 25 TSRMLS_CC, "RFC4122 UUID must be %d bytes; actually: %d", PHP_MONGO_BIN_UUID_RFC4122_SIZE, len);
					return NULL;
				}

				object_init_ex(value, mongo_ce_BinData);

				zend_update_property_stringl(mongo_ce_BinData, value, "bin", strlen("bin"), buf, len TSRMLS_CC);
				zend_update_property_long(mongo_ce_BinData, value, "type", strlen("type"), subtype TSRMLS_CC);

				buf += len;
				break;
			}

			case BSON_BOOL: {
				CHECK_BUFFER_LEN(BYTE_8);
				ZVAL_BOOL(value, *buf++);
				break;
			}

			case BSON_UNDEF:
			case BSON_NULL: {
				ZVAL_NULL(value);
				break;
			}

			case BSON_INT: {
				CHECK_BUFFER_LEN(INT_32);
				ZVAL_LONG(value, MONGO_32(*((int32_t*)buf)));
				buf += INT_32;
				break;
			}

			case BSON_LONG: {
				int force_as_object = BSON_OPT_DONT_FORCE_LONG_AS_OBJECT;

				if (options && options->flag_cmd_cursor_as_int64 && ((options->level == 1 && strcmp(name, "id") == 0) || (options->level == 3 && strcmp(name, "id") == 0))) {
					force_as_object = BSON_OPT_FORCE_LONG_AS_OBJECT;
				}
				CHECK_BUFFER_LEN(INT_64);
				php_mongo_handle_int64(
					&value,
					MONGO_64(*((int64_t*)buf)),
					force_as_object
					TSRMLS_CC
				);
				buf += INT_64;
				break;
			}

			case BSON_DATE: {
				int64_t d;

				CHECK_BUFFER_LEN(INT_64);

				d = MONGO_64(*((int64_t*)buf));
				buf += INT_64;

				object_init_ex(value, mongo_ce_Date);
				php_mongo_date_init(value, d TSRMLS_CC);

				break;
			}

			case BSON_REGEX: {
				const char *regex, *flags;
				int regex_len, flags_len;

				regex = buf;
				regex_len = strlen(buf);
				CHECK_BUFFER_LEN(regex_len + 1);

				/* get past pattern */
				buf += regex_len + 1;

				flags = buf;
				flags_len = strlen(buf);
				CHECK_BUFFER_LEN(flags_len + 1);

				/* get past flags */
				buf += flags_len + 1;

				object_init_ex(value, mongo_ce_Regex);

				zend_update_property_stringl(mongo_ce_Regex, value, "regex", strlen("regex"), regex, regex_len TSRMLS_CC);
				zend_update_property_stringl(mongo_ce_Regex, value, "flags", strlen("flags"), flags, flags_len TSRMLS_CC);

				break;
			}

			case BSON_CODE:
			case BSON_CODE__D: {
				zval *zcope;
				int code_len;
				const char *code;

				/* CODE has a useless total size field */
				if (type == BSON_CODE) {
					buf += INT_32;
				}

				CHECK_BUFFER_LEN(INT_32);

				code_len = MONGO_32(*(int32_t*)buf);
				buf += INT_32;

				/* length of code (includes \0) */
				if (code_len < 1) {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 24 TSRMLS_CC, "invalid code length for key \"%s\": %d", name, code_len);
					return NULL;
				}

				CHECK_BUFFER_LEN(code_len);

				/* ensure that string is null-terminated */
				if (buf[code_len - 1] != '\0') {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 41 TSRMLS_CC, "code string for key \"%s\" is not null-terminated", name);
					return NULL;
				}

				code = buf;
				buf += code_len;

				if (type == BSON_CODE) {
					int scope_len;

					/* Peek at the scope's document length before recursing */
					CHECK_BUFFER_LEN(INT_32);
					scope_len = MONGO_32(*(int32_t*)buf);

					CHECK_BUFFER_LEN(scope_len);

					/* initialize scope array */
					MAKE_STD_ZVAL(zcope);
					array_init(zcope);

					buf = bson_to_zval(buf, scope_len, HASH_P(zcope), options TSRMLS_CC);
					if (EG(exception)) {
						zval_ptr_dtor(&value);
						zval_ptr_dtor(&zcope);
						return NULL;
					}
				} else {
					/* initialize an empty scope array */
					MAKE_STD_ZVAL(zcope);
					array_init(zcope);
				}

				object_init_ex(value, mongo_ce_Code);
				/* exclude \0 */
				zend_update_property_stringl(mongo_ce_Code, value, "code", strlen("code"), code, code_len-1 TSRMLS_CC);
				zend_update_property(mongo_ce_Code, value, "scope", strlen("scope"), zcope TSRMLS_CC);
				zval_ptr_dtor(&zcope);

				break;
			}

			/* DEPRECATED
			 *
			 * This is a deprecated BSON type (since early 1.x server versions)
			 * used for referencing another document. It consists of:
			 *
			 *   - 4-byte namespace length (includes trailing \0)
			 *   - namespace + \0
			 *   - 12-byte ObjectId
			 *
			 * Since there is no PHP class for this type, we silently convert it
			 * to a DBRef document (e.g. ['$ref' => ..., '$id' => ... ]).
			 *
			 * See: http://docs.mongodb.org/manual/reference/database-references/#dbrefs
			 */
			case BSON_DBPOINTER: {
				int ns_len;
				zval *zoid;
				const char *ns;
				char *str = NULL;
				mongo_id *this_id;

				CHECK_BUFFER_LEN(INT_32);

				ns_len = MONGO_32(*(int32_t*)buf);
				buf += INT_32;

				/* length of namespace (includes \0) */
				if (ns_len < 1) {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 3 TSRMLS_CC, "invalid DBPointer namespace length for key \"%s\": %d", name, ns_len);
					return NULL;
				}

				CHECK_BUFFER_LEN(ns_len);

				/* ensure that string is null-terminated */
				if (buf[ns_len - 1] != '\0') {
					zval_ptr_dtor(&value);
					zend_throw_exception_ex(mongo_ce_CursorException, 41 TSRMLS_CC, "DBPointer namespace string for key \"%s\" is not null-terminated", name);
					return NULL;
				}

				ns = buf;
				buf += ns_len;

				CHECK_BUFFER_LEN(OID_SIZE);

				MAKE_STD_ZVAL(zoid);
				object_init_ex(zoid, mongo_ce_Id);

				this_id = (mongo_id*)zend_object_store_get_object(zoid TSRMLS_CC);
				this_id->id = estrndup(buf, OID_SIZE);

				str = php_mongo_mongoid_to_hex(this_id->id);
				zend_update_property_string(mongo_ce_Id, zoid, "$id", strlen("$id"), str TSRMLS_CC);
				efree(str);

				buf += OID_SIZE;

				/* put it all together */
				array_init(value);
				add_assoc_stringl(value, "$ref", (char*) ns, ns_len-1, 1);
				add_assoc_zval(value, "$id", zoid);
				break;
			}

			/* MongoTimestamp (17)
			 * 8 bytes total:
			 *  - sec: 4 bytes
			 *  - inc: 4 bytes */
			case BSON_TIMESTAMP: {
				CHECK_BUFFER_LEN(INT_64);
				object_init_ex(value, mongo_ce_Timestamp);
				zend_update_property_long(mongo_ce_Timestamp, value, "inc", strlen("inc"), MONGO_32(*(int32_t*)buf) TSRMLS_CC);
				buf += INT_32;
				zend_update_property_long(mongo_ce_Timestamp, value, "sec", strlen("sec"), MONGO_32(*(int32_t*)buf) TSRMLS_CC);
				buf += INT_32;
				break;
			}

			/* max and min keys are used only for sharding, and
			 * cannot be resaved to the database at the moment */
			/* MongoMinKey (0) */
			case BSON_MINKEY: {
				object_init_ex(value, mongo_ce_MinKey);
				break;
			}

			/* MongoMinKey (127) */
			case BSON_MAXKEY: {
				object_init_ex(value, mongo_ce_MaxKey);
				break;
			}

			default: {
				/* If we run into a type we don't recognize, it's possible that
				 * there is data corruption or we've encountered an unsupported
				 * BSON type. Either way, it's helpful to know the situation
				 * that led us here, so include a hex dump of the BSON buffer up
				 * to this point in the exception message.
				 *
				 * We can't dump any more of the buffer, unfortunately, because we
				 * don't keep track of the size.  Besides, if it is corrupt, the
				 * size might be messed up, too. */
				char *msg, *pos, *template;
				int i, hex_byte_len, buf_dump_len, template_len;
				size_t msg_len;

				template = "Detected unknown BSON type 0x%02hhx for fieldname \"%s\". If this is an unsupported type and not data corruption, consider upgrading to the \"mongodb\" extension. BSON buffer:";

				/* Each dumped byte is 3 characters (e.g. " ff") */
				hex_byte_len = 3;
				buf_dump_len = (buf - buf_start) * hex_byte_len;

				/* Subtract from template's length to account for the difference
				 * between the format directives and formatted output. We
				 * substract 6 because:
				 *
				 *  * %02hhx is 4 extra characters (replaced with 2 characters)
				 *  * %s is 2 extra characters (replaced by name, and we already
				 *    count name_len)
				 *
				 * Finally, add 1 for a terminating null byte. */
				msg_len = strlen(template) - 6 + name_len + buf_dump_len + 1;
				msg = (char*) emalloc(msg_len);
				template_len = snprintf(msg, msg_len, template, type, name);

				/* Jump to the end of the printed template to dump BSON */
				pos = msg + template_len;
				for (i = 0; i < buf - buf_start; i++) {
					snprintf(pos, hex_byte_len + 1, " %02hhx", (unsigned char) buf_start[i]);
					pos += hex_byte_len;
				}
				/* Ensure the message is null-terminated */
				msg[msg_len - 1] = '\0';

				zval_ptr_dtor(&value);
				zend_throw_exception(mongo_ce_Exception, msg, 17 TSRMLS_CC);
				efree(msg);
				return NULL;
			}
		}

		zend_symtable_update(result, name, strlen(name) + 1, &value, sizeof(zval*), NULL);
	}

	return buf;
}

static int is_utf8(const char *s, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i + 3 < len && (s[i] & 248) == 240 && (s[i + 1] & 192) == 128 && (s[i + 2] & 192) == 128 && (s[i + 3] & 192) == 128) {
			i += 3;
		} else if (i + 2 < len && (s[i] & 240) == 224 && (s[i + 1] & 192) == 128 && (s[i + 2] & 192) == 128) {
			i += 2;
		} else if (i + 1 < len && (s[i] & 224) == 192 && (s[i + 1] & 192) == 128) {
			i += 1;
		} else if ((s[i] & 128) != 0) {
			return 0;
		}
	}
	return 1;
}

/* {{{ proto string bson_encode(mixed document)
   Takes any type of PHP var and turns it into BSON */
PHP_FUNCTION(bson_encode)
{
	zval *z;
	mongo_buffer buf;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z) == FAILURE) {
		return;
	}

	switch (Z_TYPE_P(z)) {
		case IS_NULL: {
			RETURN_STRING("", 1);
			break;
		}

		case IS_LONG: {
			CREATE_BUF_STATIC(9);
#if SIZEOF_LONG == 4
			php_mongo_serialize_int(&buf, Z_LVAL_P(z));
			RETURN_STRINGL(buf.start, 4, 1);
#else
			php_mongo_serialize_long(&buf, Z_LVAL_P(z));
			RETURN_STRINGL(buf.start, 8, 1);
#endif
			break;
		}

		case IS_DOUBLE: {
			CREATE_BUF_STATIC(9);
			php_mongo_serialize_double(&buf, Z_DVAL_P(z));
			RETURN_STRINGL(b, 8, 1);
			break;
		}

		case IS_BOOL: {
			if (Z_BVAL_P(z)) {
				RETURN_STRINGL("\x01", 1, 1);
			} else {
				RETURN_STRINGL("\x00", 1, 1);
			}
			break;
		}

		case IS_STRING: {
			RETURN_STRINGL(Z_STRVAL_P(z), Z_STRLEN_P(z), 1);
			break;
		}

		case IS_OBJECT: {
			zend_class_entry *clazz = Z_OBJCE_P(z);

			if (clazz == mongo_ce_Id) {
				mongo_id *id = (mongo_id*)zend_object_store_get_object(z TSRMLS_CC);
				RETURN_STRINGL(id->id, 12, 1);
				break;
			} else if (clazz == mongo_ce_Date) {
				CREATE_BUF_STATIC(9);
				php_mongo_serialize_date(&buf, z TSRMLS_CC);
				RETURN_STRINGL(buf.start, 8, 1);
				break;
			} else if (clazz == mongo_ce_Regex) {
				CREATE_BUF(buf, 128);

				php_mongo_serialize_regex(&buf, z TSRMLS_CC);
				RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
				efree(buf.start);
				break;
			} else if (clazz == mongo_ce_Code) {
				CREATE_BUF(buf, INITIAL_BUF_SIZE);

				php_mongo_serialize_code(&buf, z TSRMLS_CC);
				RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
				efree(buf.start);
				break;
			} else if (clazz == mongo_ce_BinData) {
				CREATE_BUF(buf, INITIAL_BUF_SIZE);

				php_mongo_serialize_bin_data(&buf, z TSRMLS_CC);
				RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
				efree(buf.start);
				break;
			} else if (clazz == mongo_ce_Timestamp) {
				CREATE_BUF_STATIC(9);
				php_mongo_serialize_ts(&buf, z TSRMLS_CC);
				RETURN_STRINGL(buf.start, 8, 1);
				break;
			} else if (clazz == mongo_ce_MaxKey || clazz == mongo_ce_MinKey) {
				RETURN_STRING("", 1);
				break;
			}
		}

		/* fallthrough for a normal obj */
		case IS_ARRAY: {
			CREATE_BUF(buf, INITIAL_BUF_SIZE);
			zval_to_bson(&buf, HASH_P(z), 0, MONGO_DEFAULT_MAX_MESSAGE_SIZE TSRMLS_CC);

			RETVAL_STRINGL(buf.start, buf.pos-buf.start, 1);
			efree(buf.start);
			break;
		}

		default:
			zend_throw_exception(zend_exception_get_default(TSRMLS_C), "couldn't serialize element", 0 TSRMLS_CC);
			return;
	}
}
/* }}} */

/* {{{ proto array bson_decode(string bson)
   Takes a serialized BSON object and turns it into a PHP array. This only deserializes entire documents! */
PHP_FUNCTION(bson_decode)
{
	const char *str;
	int str_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
		return;
	}

	array_init(return_value);
	bson_to_zval(str, str_len, HASH_P(return_value), 0 TSRMLS_CC);
}
/* }}} */

void mongo_buf_init(char *dest)
{
	dest[0] = '\0';
}

void mongo_buf_append(char *dest, const char *piece)
{
	int pos = strlen(dest);
	memcpy(dest + pos, piece, strlen(piece) + 1);
}

void php_mongo_handle_int64(zval **value, int64_t nr, int force_options TSRMLS_DC)
{
	if (
		(force_options == BSON_OPT_FORCE_LONG_AS_OBJECT || MonGlo(long_as_object))
#if SIZEOF_LONG == 4
		||
		(force_options == BSON_OPT_INT32_LONG_AS_OBJECT)
#endif
	) {
		char *tmp_string;

#ifdef WIN32
		spprintf(&tmp_string, 0, "%I64d", (int64_t)nr);
#else
		spprintf(&tmp_string, 0, "%lld", (long long int)nr);
#endif
		object_init_ex(*value, mongo_ce_Int64);

		zend_update_property_string(mongo_ce_Int64, *value, "value", strlen("value"), tmp_string TSRMLS_CC);

		efree(tmp_string);
	} else {
#if SIZEOF_LONG == 4
		if (nr <= LONG_MAX && nr >= -LONG_MAX - 1) {
			ZVAL_LONG(*value, (long)nr);
			return;
		}
		zend_throw_exception_ex(mongo_ce_CursorException, 23 TSRMLS_CC, "Cannot natively represent the long %lld on this platform", (int64_t)nr);
		zval_ptr_dtor(value);
		return;
#else
# if SIZEOF_LONG == 8
		if (MonGlo(native_long)) {
			ZVAL_LONG(*value, (long)nr);
		} else {
			ZVAL_DOUBLE(*value, (double)nr);
		}
# else
#  error The PHP number size is neither 4 or 8 bytes; no clue what to do with that!
# endif
#endif
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
