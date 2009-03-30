// bson.cpp
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

#include "bson.h"
#include "mongo.h"
#include "mongo_types.h"

extern zend_class_entry *mongo_bindata_class;
extern zend_class_entry *mongo_code_class;
extern zend_class_entry *mongo_date_class;
extern zend_class_entry *mongo_id_class;
extern zend_class_entry *mongo_regex_class;


char* prep_obj_for_db(char *buf, char *size, HashTable *array TSRMLS_DC) {
  zval **data;

  // array is of the form: 
  // array("query" => array("_id" : MongoId), "orderby" => ...)

  // get query
  // 6 is length of "query" + 1 for \0
  if (zend_hash_find(array, "query", 6, (void**)&data) == SUCCESS) {
    array = Z_ARRVAL_PP(data);
  }

  // check if query._id field doesn't exist, add it
  if (zend_hash_find(array, "_id", 4, (void**)&data) == FAILURE) {
    char foo[12];
    create_id(foo);
    buf = set_type(buf, size, BSON_OID);
    buf = serialize_string(buf, size, "_id", 3);
    return serialize_string(buf, size, foo, OID_SIZE);
  }

  // if it exists, it'll be serialized
  return buf;
}


// serialize a zval
char* zval_to_bson(char *buf, char *size, HashTable *arr_hash TSRMLS_DC) {
  zval **data;
  char *key;
  uint key_len;
  ulong index;
  zend_bool duplicate = 0;
  HashPosition pointer;
  int num = 0;
  char *start = buf;

  // check buf size
  if(size-buf < 5) {
    size = resize_buf(buf, size);
  }

  // skip first 4 bytes to leave room for size
  buf += INT_32;

  if (zend_hash_num_elements(arr_hash) == 0) {
    buf = serialize_null(buf, size);
    buf = serialize_size(start, buf);
    return buf;
  }
 
  
  for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer); 
      zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(arr_hash, &pointer)) {

    // increment number of fields
    num++;

    int key_type = zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, duplicate, &pointer);
    char *field_name;

    if (key_type == HASH_KEY_IS_STRING) {
      key_len = spprintf(&field_name, 0, "%s", key);
    }
    else if (key_type == HASH_KEY_IS_LONG) {
      key_len = spprintf(&field_name, 0, "%ld", index);
    }
    else {
      zend_error(E_WARNING, "key fail");
      continue;
    }

    if(size-buf < key_len+2) {
      size = resize_buf(buf, size);
    }
    buf = serialize_element(buf, size, field_name, key_len, data TSRMLS_CC);
    efree(field_name);
  }
  buf = serialize_null(buf, size);
  serialize_size(start, buf);
  return buf;
}

char* serialize_element(char *buf, char *size, char *name, int name_len, zval **data TSRMLS_DC) {

  switch (Z_TYPE_PP(data)) {
  case IS_NULL:
    buf = set_type(buf, size, BSON_NULL);
    buf = serialize_string(buf, size, name, name_len);
    break;
  case IS_LONG:
    buf = set_type(buf, size, BSON_INT);
    buf = serialize_string(buf, size, name, name_len);
    buf = serialize_int(buf, size, Z_LVAL_PP(data));
    break;
  case IS_DOUBLE:
    buf = set_type(buf, size, BSON_DOUBLE);
    buf = serialize_string(buf, size, name, name_len);
    buf = serialize_double(buf, size, Z_DVAL_PP(data));
    break;
  case IS_BOOL:
    buf = set_type(buf, size, BSON_BOOL);
    buf = serialize_string(buf, size, name, name_len);
    buf = serialize_bool(buf, size, Z_BVAL_PP(data));
    break;
  case IS_STRING: {
    buf = set_type(buf, size, BSON_STRING);
    buf = serialize_string(buf, size, name, name_len);

    //while(1)kiss(kristina);
    long length = Z_STRLEN_PP(data);
    long length0 = length + BYTE_8;
    memcpy(buf, &length0, INT_32);
    buf += INT_32;

    buf = serialize_string(buf, size, Z_STRVAL_PP(data), length);
    break;
  }
  case IS_ARRAY: {
    buf = set_type(buf, size, BSON_OBJECT);
    buf = serialize_string(buf, size, name, name_len);
    buf = zval_to_bson(buf, size, Z_ARRVAL_PP(data) TSRMLS_CC);
    break;
  }
  case IS_OBJECT: {
    zend_class_entry *clazz = Z_OBJCE_PP( data );
    /* check for defined classes */
    // MongoId
    if(clazz == mongo_id_class) {
      buf = set_type(buf, size, BSON_OID);
      buf = serialize_string(buf, size, name, name_len);
      zval *zid = zend_read_property(mongo_id_class, *data, "id", 2, 0 TSRMLS_CC);
      buf = serialize_string(buf, size, Z_STRVAL_P(zid), OID_SIZE);
    }
    // MongoDate
    else if (clazz == mongo_date_class) {
      buf = set_type(buf, size, BSON_DATE);
      buf = serialize_string(buf, size, name, name_len);
      zval *zid = zend_read_property(mongo_date_class, *data, "ms", 2, 0 TSRMLS_CC);
      buf = serialize_long(buf, size, Z_LVAL_P(zid));
    }
    // MongoRegex
    else if (clazz == mongo_regex_class) {
      buf = set_type(buf, size, BSON_REGEX);
      buf = serialize_string(buf, size, name, name_len);
      zval *zid = zend_read_property(mongo_regex_class, *data, "regex", 5, 0 TSRMLS_CC);
      buf = serialize_string(buf, size, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
      zid = zend_read_property(mongo_regex_class, *data, "flags", 5, 0 TSRMLS_CC);
      buf = serialize_string(buf, size, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
    }
    // MongoCode
    else if (clazz == mongo_code_class) {
      buf = set_type(buf, size, BSON_CODE);
      buf = serialize_string(buf, size, name, name_len);

      // save spot for size
      char* start = buf;
      buf += INT_32;
      zval *zid = zend_read_property(mongo_code_class, *data, "code", 4, 0 TSRMLS_CC);
      // string size
      buf = serialize_int(buf, size, Z_STRLEN_P(zid)+1);
      // string
      buf = serialize_string(buf, size, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
      // scope
      zid = zend_read_property(mongo_code_class, *data, "scope", 5, 0 TSRMLS_CC);
      buf = zval_to_bson(buf, size, Z_ARRVAL_P(zid) TSRMLS_CC);

      // get total size
      serialize_size(start, buf);
    }
    // MongoBin
    else if (clazz == mongo_bindata_class) {
      buf = set_type(buf, size, BSON_BINARY);
      buf = serialize_string(buf, size, name, name_len);

      zval *zid = zend_read_property(mongo_bindata_class, *data, "length", 6, 0 TSRMLS_CC);
      buf = serialize_int(buf, size, Z_LVAL_P(zid));
      zid = zend_read_property(mongo_bindata_class, *data, "type", 4, 0 TSRMLS_CC);
      buf = serialize_byte(buf, size, (char)Z_LVAL_P(zid));
      zid = zend_read_property(mongo_bindata_class, *data, "bin", 3, 0 TSRMLS_CC);
      buf = serialize_string(buf, size, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
    }
    break;
  }
  }
  return buf;
}

char* resize_buf(char *buf, char *size) {
  int total = size - buf;
  total = total < GROW_SLOWLY ? total*2 : total+INITIAL_BUF_SIZE;

  buf = (char*)erealloc(buf, total);
  return buf;
}

char* serialize_byte(char *buf, char *size, char b) {
  if(size-buf < 1) {
    size = resize_buf(buf, size);
  }
  *(buf) = b;
  return buf += 1;
}

char* serialize_string(char *buf, char *size, char *str, int str_len) {
  if(size-buf < str_len+1) {
    size = resize_buf(buf, size);
  }
  memcpy(buf, str, str_len);
  // add \0 at the end of the string
  buf[str_len] = 0;
  return buf += str_len + 1;
}

char* serialize_int(char *buf, char *size, int num) {
  if(size-buf < INT_32) {
    size = resize_buf(buf, size);
  }
  memcpy(buf, &num, INT_32);
  return buf += INT_32;
}

char* serialize_long(char *buf, char *size, long num) {
  if(size-buf < INT_64) {
    size = resize_buf(buf, size);
  }
  memcpy(buf, &num, INT_64);
  return buf += INT_64;
}

char* serialize_double(char *buf, char *size, double num) {
  if(size-buf < INT_64) {
    size = resize_buf(buf, size);
  }
  memcpy(buf, &num, DOUBLE_64);
  return buf += DOUBLE_64;
}

/* the position is not increased, we are just filling
 * in the first 4 bytes with the size.
 */
char* serialize_size(char *start, char *end) {
  int total = end-start;
  memcpy(start, &total, INT_32);
  return end;
}


char* bson_to_zval(char *buf, zval *result TSRMLS_DC) {

  int size;
  memcpy(&size, buf, INT_32);
  buf += INT_32;

  char type;
  while (type = *buf++) {
    int name_len = 0;
    // get field name
    char name[64];
    while (name[name_len++] = *buf++);
    // move past \0 at end
    name[name_len] = 0;

    // get value
    switch(type) {
    case BSON_OID: {
      zval *zoid;
      MAKE_STD_ZVAL(zoid);
      object_init_ex(zoid, mongo_id_class);
      char *id = (char*)emalloc(12);
      int i=0;
      for(i=0;i<12;i++) {
        *(id) = *buf++;
      }
      add_property_stringl(zoid, "id", id, 12, 1);
      efree(id);
      add_assoc_zval(result, name, zoid);
      break;
    }
    case BSON_DOUBLE: {
      double d;
      memcpy(&d, buf, DOUBLE_64);
      add_assoc_double(result, name, d);
      buf += DOUBLE_64;
      break;
    }
    case BSON_STRING: {
      int len, duplicate = 1;
      memcpy(&len, buf, INT_32);
      buf += INT_32;
 
      char str[len];
      int i=0;
      while (str[i++] = *buf++);
      str[i] = 0;
      add_assoc_stringl(result, name, str, len-1, duplicate);
      break;
    }
    case BSON_OBJECT:
    case BSON_ARRAY: {
      zval *d;
      ALLOC_INIT_ZVAL(d);
      array_init(d);
      buf = bson_to_zval(buf, d TSRMLS_CC);
      add_assoc_zval(result, name, d);
      break;
    }
    case BSON_BINARY: {
      int len;
      memcpy(&len, buf, INT_32);
      buf += INT_32;

      char type = *buf++;

      char *bytes = (char*)emalloc(len+1);
      int i=0;
      for(i=0;i<len;i++) {
        bytes[i] = *buf++;
      }
      bytes[len] = 0;

      zval *bin;
      MAKE_STD_ZVAL(bin);
      object_init_ex(bin, mongo_bindata_class);

      add_property_long(bin, "length", len);
      add_property_stringl(bin, "bin", bytes, len, 0);
      add_property_long(bin, "type", type);
      add_assoc_zval(result, name, bin);
      break;
    }
    case BSON_BOOL: {
      char d = *buf++;
      add_assoc_bool(result, name, d);
      break;
    }
    case BSON_UNDEF:
    case BSON_NULL: {
      add_assoc_null(result, name);
      break;
    }
    case BSON_INT: {
      long d;
      memcpy(&d, buf, INT_32);
      add_assoc_long(result, name, d);
      buf += INT_32;
      break;
    }
    case BSON_DATE: {
      long d;
      memcpy(&d, buf, INT_64);
      buf += INT_64;

      zval *date;
      MAKE_STD_ZVAL(date);
      object_init_ex(date, mongo_date_class);

      add_property_long(date, "ms", d);
      add_assoc_zval(result, name, date);
      break;
    }
    case BSON_REGEX: {
      char *start_regex = buf;
      while (*buf++);
      int regex_len = buf-start_regex;

      char *start_flags = buf;
      while (*buf++);
      int flags_len = buf-start_flags;

      zval *zegex;
      MAKE_STD_ZVAL(zegex);
      object_init_ex(zegex, mongo_regex_class);

      add_property_stringl(zegex, "regex", start_regex, regex_len, 1);
      add_property_stringl(zegex, "flags", start_flags, flags_len, 1);
      add_assoc_zval(result, name, zegex);

      break;
    }
    case BSON_CODE: 
    case BSON_CODE__D: {
      zval *zode, *zcope;
      MAKE_STD_ZVAL(zode);
      object_init_ex(zode, mongo_code_class);
      MAKE_STD_ZVAL(zcope);
      array_init(zcope);

      if (type == BSON_CODE) {
        buf += INT_32;
      }

      int len;
      memcpy(&len, buf, INT_32);
      buf += INT_32;
 
      char str[len];
      int i=0;
      while (str[i++] = *buf++);
      str[i] = 0;

      if (type == BSON_CODE) {
        buf = bson_to_zval(buf, zcope TSRMLS_CC);
      }

      add_property_stringl(zode, "code", str, len, DUP);
      add_property_zval(zode, "scope", zcope);
      add_assoc_zval(result, name, zode);
      break;
    }
    default: {
      php_printf("type %d not supported\n", type);
    }
    }
  }
  return buf;
}

