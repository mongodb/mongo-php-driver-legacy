// bson.c
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

#ifdef WIN32
#include <memory.h>
#endif

#include "bson.h"
#include "mongo.h"
#include "mongo_types.h"

extern zend_class_entry *mongo_bindata_class;
extern zend_class_entry *mongo_code_class;
extern zend_class_entry *mongo_date_class;
extern zend_class_entry *mongo_id_class;
extern zend_class_entry *mongo_regex_class;


int prep_obj_for_db(buffer *buf, HashTable *array TSRMLS_DC) {
  zval **data;
 
  // if _id field doesn't exist, add it
  if (zend_hash_find(array, "_id", 4, (void**)&data) == FAILURE) {
    zval *z;
    MAKE_STD_ZVAL(z);
    create_id(z, 0 TSRMLS_CC);
    serialize_element(buf, "_id", 3, &z TSRMLS_CC);
    efree(z);
  }
  else {
    serialize_element(buf, "_id", 3, data TSRMLS_CC);
    zend_hash_del(array, "_id", 4);
  }
  return SUCCESS;
}


// serialize a zval
int zval_to_bson(buffer *buf, HashTable *arr_hash, int prep TSRMLS_DC) {
  zval **data;
  char *key;
  uint key_len;
  ulong index;
  HashPosition pointer;
  int num = 0;

  // check buf size
  if(BUF_REMAINING <= 5) {
    resize_buf(buf, 5);
  }

  // keep a record of the starting position
  // as an offset, in case the memory is resized
  unsigned int start = buf->pos-buf->start;

  // skip first 4 bytes to leave room for size
  buf->pos += INT_32;

  if (zend_hash_num_elements(arr_hash) == 0) {
    serialize_null(buf);
    serialize_size(buf->start+start, buf);
    return num;
  }
 

  if (prep) {
    prep_obj_for_db(buf, arr_hash TSRMLS_CC);
  }
  
  for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer); 
      zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(arr_hash, &pointer)) {

    // increment number of fields
    num++;

    int key_type = zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, NO_DUP, &pointer);
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

    serialize_element(buf, field_name, key_len, data TSRMLS_CC);
    efree(field_name);
  }
  serialize_null(buf);
  serialize_size(buf->start+start, buf);
  return num;
}

void serialize_element(buffer *buf, char *name, int name_len, zval **data TSRMLS_DC) {

  switch (Z_TYPE_PP(data)) {
  case IS_NULL:
    set_type(buf, BSON_NULL);
    serialize_string(buf, name, name_len);
    break;
  case IS_LONG:
    set_type(buf, BSON_INT);
    serialize_string(buf, name, name_len);
    serialize_int(buf, Z_LVAL_PP(data));
    break;
  case IS_DOUBLE:
    set_type(buf, BSON_DOUBLE);
    serialize_string(buf, name, name_len);
    serialize_double(buf, Z_DVAL_PP(data));
    break;
  case IS_BOOL:
    set_type(buf, BSON_BOOL);
    serialize_string(buf, name, name_len);
    serialize_bool(buf, Z_BVAL_PP(data));
    break;
  case IS_STRING: {
    set_type(buf, BSON_STRING);
    serialize_string(buf, name, name_len);

    long length = Z_STRLEN_PP(data);
    serialize_int(buf, length+1);
    serialize_string(buf, Z_STRVAL_PP(data), length);
    break;
  }
  case IS_ARRAY: {
    if (zend_hash_index_exists(Z_ARRVAL_PP(data), 0)) {
      set_type(buf, BSON_ARRAY);
    }
    else {
      set_type(buf, BSON_OBJECT);
    }

    serialize_string(buf, name, name_len);
    zval_to_bson(buf, Z_ARRVAL_PP(data), NO_PREP TSRMLS_CC);
    break;
  }
  case IS_OBJECT: {
    zend_class_entry *clazz = Z_OBJCE_PP( data );
    /* check for defined classes */
    // MongoId
    if(clazz == mongo_id_class) {
      set_type(buf, BSON_OID);
      serialize_string(buf, name, name_len);
      zval *zid = zend_read_property(mongo_id_class, *data, "id", 2, 0 TSRMLS_CC);
      memcpy(buf->pos, Z_STRVAL_P(zid), OID_SIZE);
      buf->pos += OID_SIZE;
    }
    // MongoDate
    else if (clazz == mongo_date_class) {
      set_type(buf, BSON_DATE);
      serialize_string(buf, name, name_len);

      zval *zsec = zend_read_property(mongo_date_class, *data, "sec", 3, 0 TSRMLS_CC);
      long sec = Z_LVAL_P(zsec);
      zval *zusec = zend_read_property(mongo_date_class, *data, "usec", 4, 0 TSRMLS_CC);
      long usec = Z_LVAL_P(zusec);
      long ms = (sec * 1000) + (usec / 1000);
      serialize_long(buf, ms);
    }
    // MongoRegex
    else if (clazz == mongo_regex_class) {
      set_type(buf, BSON_REGEX);
      serialize_string(buf, name, name_len);
      zval *zid = zend_read_property(mongo_regex_class, *data, "regex", 5, 0 TSRMLS_CC);
      serialize_string(buf, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
      zid = zend_read_property(mongo_regex_class, *data, "flags", 5, 0 TSRMLS_CC);
      serialize_string(buf, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
    }
    // MongoCode
    else if (clazz == mongo_code_class) {
      set_type(buf, BSON_CODE);
      serialize_string(buf, name, name_len);

      // save spot for size
      unsigned int start = buf->pos-buf->start;
      buf->pos += INT_32;
      zval *zid = zend_read_property(mongo_code_class, *data, "code", 4, 0 TSRMLS_CC);
      // string size
      serialize_int(buf, Z_STRLEN_P(zid)+1);
      // string
      serialize_string(buf, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
      // scope
      zid = zend_read_property(mongo_code_class, *data, "scope", 5, 0 TSRMLS_CC);
      zval_to_bson(buf, Z_ARRVAL_P(zid), NO_PREP TSRMLS_CC);

      // get total size
      serialize_size(buf->start+start, buf);
    }
    // MongoBin
    else if (clazz == mongo_bindata_class) {
      set_type(buf, BSON_BINARY);
      serialize_string(buf, name, name_len);

      zval *zid = zend_read_property(mongo_bindata_class, *data, "length", 6, 0 TSRMLS_CC);
      serialize_int(buf, Z_LVAL_P(zid));
      zid = zend_read_property(mongo_bindata_class, *data, "type", 4, 0 TSRMLS_CC);
      serialize_byte(buf, (unsigned char)Z_LVAL_P(zid));
      zid = zend_read_property(mongo_bindata_class, *data, "bin", 3, 0 TSRMLS_CC);
      serialize_string(buf, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
    }
    break;
  }
  }
}

int resize_buf(buffer *buf, int size) {
  unsigned int total = buf->end - buf->start;
  unsigned int pos = buf->pos - buf->start;
  total = total < GROW_SLOWLY ? total*2 : total+INITIAL_BUF_SIZE;
  if (total < size) {
    total += size;
  }

  buf->start = (unsigned char*)erealloc(buf->start, total);
  buf->pos = buf->start + pos;
  buf->end = buf->start + total;
  return total;
}

void serialize_byte(buffer *buf, char b) {
  if(BUF_REMAINING <= 1) {
    resize_buf(buf, 1);
  }
  *(buf->pos) = b;
  buf->pos += 1;
}

void serialize_string(buffer *buf, char *str, int str_len) {
  if(BUF_REMAINING <= str_len+1) {
    resize_buf(buf, str_len+1);
  }
  memcpy(buf->pos, str, str_len);
  // add \0 at the end of the string
  buf->pos[str_len] = 0;
  buf->pos += str_len + 1;
}

void serialize_int(buffer *buf, int num) {
  if(BUF_REMAINING <= INT_32) {
    resize_buf(buf, INT_32);
  }
  memcpy(buf->pos, &num, INT_32);
  buf->pos += INT_32;
}

void serialize_long(buffer *buf, long long num) {
  if(BUF_REMAINING <= INT_64) {
    resize_buf(buf, INT_64);
  }
  memcpy(buf->pos, &num, INT_64);
  buf->pos += INT_64;
}

void serialize_double(buffer *buf, double num) {
  if(BUF_REMAINING <= INT_64) {
    resize_buf(buf, INT_64);
  }
  memcpy(buf->pos, &num, DOUBLE_64);
  buf->pos += DOUBLE_64;
}

/* the position is not increased, we are just filling
 * in the first 4 bytes with the size.
 */
void serialize_size(unsigned char *start, buffer *buf) {
  unsigned int total = buf->pos - start;
  memcpy(start, &total, INT_32);
}


unsigned char* bson_to_zval(unsigned char *buf, zval *result TSRMLS_DC) {
  unsigned int size;
  memcpy(&size, buf, INT_32);
  buf += INT_32;
  size -= INT_32;

  unsigned char type;
  // size is for sanity check
  while (size-- > 0 && (type = *buf++)) {
    char* name = (char*)buf;
    // get past field name
    while (*buf++);

    // get value
    switch(type) {
    case BSON_OID: {
      zval *z;
      MAKE_STD_ZVAL(z);
      create_id(z, (char*)buf TSRMLS_CC);
      add_assoc_zval(result, name, z);
      buf += OID_SIZE;
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
      int len;
      memcpy(&len, buf, INT_32);
      buf += INT_32;

      add_assoc_stringl(result, name, (char*)buf, len-1, DUP);
      buf += len;
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

      unsigned char type = *buf++;

      unsigned char *bytes = buf;
      buf += len;

      zval *bin;
      MAKE_STD_ZVAL(bin);
      object_init_ex(bin, mongo_bindata_class);

      add_property_long(bin, "length", len);
      add_property_stringl(bin, "bin", (char*)bytes, len, DUP);
      add_property_long(bin, "type", type);
      add_assoc_zval(result, name, bin);
      break;
    }
    case BSON_BOOL: {
      unsigned char d = *buf++;
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
      unsigned long long d;
      memcpy(&d, buf, INT_64);
      buf += INT_64;

      zval *date;
      MAKE_STD_ZVAL(date);
      object_init_ex(date, mongo_date_class);

      add_property_long(date, "sec", d/1000);
      add_property_long(date, "usec", (d*1000)%1000000);
      add_assoc_zval(result, name, date);
      break;
    }
    case BSON_REGEX: {
      unsigned char *start_regex = buf;
      while (*buf++);
      int regex_len = buf-start_regex;

      unsigned char *start_flags = buf;
      while (*buf++);
      int flags_len = buf-start_flags;

      zval *zegex;
      MAKE_STD_ZVAL(zegex);
      object_init_ex(zegex, mongo_regex_class);

      add_property_stringl(zegex, "regex", (char*)start_regex, regex_len-1, 1);
      add_property_stringl(zegex, "flags", (char*)start_flags, flags_len-1, 1);
      add_assoc_zval(result, name, zegex);

      break;
    }
    case BSON_CODE: 
    case BSON_CODE__D: {
      zval *zode, *zcope;
      MAKE_STD_ZVAL(zode);
      object_init_ex(zode, mongo_code_class);
      // initialize scope array
      MAKE_STD_ZVAL(zcope);
      array_init(zcope);

      // CODE has an initial size field
      if (type == BSON_CODE) {
        buf += INT_32;
      }

      // length of code (includes \0)
      int len;
      memcpy(&len, buf, INT_32);

      buf += INT_32;
      unsigned char *code = buf;
      buf += len * BYTE_8;

      if (type == BSON_CODE) {
        buf = bson_to_zval(buf, zcope TSRMLS_CC);
      }

      // exclude \0
      add_property_stringl(zode, "code", (char*)code, len-1, DUP);
      add_property_zval(zode, "scope", zcope);
      zval_ptr_dtor(&zcope);
      add_assoc_zval(result, name, zode);
      break;
    }
    case BSON_DBREF: {
      zval *zref;
      ALLOC_INIT_ZVAL(zref);
      array_init(zref);

      buf += INT_32;
      char *ns = (char*)buf;
      while (*buf++);

      zval *zoid;
      MAKE_STD_ZVAL(zoid);
      create_id(zoid, (char*)buf TSRMLS_CC);
      buf += OID_SIZE;

      add_assoc_string(zref, "$ref", ns, 1);
      add_assoc_zval(zref, "$id", zoid);

      add_assoc_zval(result, name, zref);
      break;
    }
    case BSON_TIMESTAMP: {
      long long int d;
      memcpy(&d, buf, INT_64);
      buf += INT_64;

      add_assoc_long(result, name, d);

      break;
    }
    case BSON_MINKEY: {
      add_assoc_string(result, name, "[MinKey]", 1);
      break;
    }
    case BSON_MAXKEY: {
      add_assoc_string(result, name, "[MaxKey]", 1);
      break;
    }
    default: {
      php_printf("type %d not supported\n", type);
      // give up, it'll be trouble if we keep going
      return buf;
    }
    }
  }
  return buf;
}

