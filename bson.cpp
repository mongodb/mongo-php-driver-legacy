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
#include <mongo/client/dbclient.h>

#include "bson.h"
#include "mongo_types.h"

extern zend_class_entry *mongo_bindata_class;
extern zend_class_entry *mongo_code_class;
extern zend_class_entry *mongo_date_class;
extern zend_class_entry *mongo_id_class;
extern zend_class_entry *mongo_regex_class;

// serialize a zval
int zval_to_bson(char *buf, int *pos, HashTable *arr_hash TSRMLS_DC) {
  zval **data;
  char *key;
  uint key_len;
  ulong index;
  zend_bool duplicate = 0;
  HashPosition pointer;
  int num = 0, start = *pos;

  // skip first 4 bytes to leave room for size
  *(pos) += INT_32;

  if (zend_hash_num_elements(arr_hash) == 0) {
    serialize_null(buf, pos);
    serialize_size(buf, start, *pos);
    return *pos;
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

    serialize_element(buf, pos, field_name, key_len, data TSRMLS_CC);
    efree(field_name);
  }
  serialize_null(buf, pos);
  serialize_size(buf, start, *pos);
  return *pos;
}

int serialize_element(char *buf, int *pos, char *name, int name_len, zval **data TSRMLS_DC) {
  switch (Z_TYPE_PP(data)) {
  case IS_NULL:
    set_type(buf, pos, BSON_NULL);
    serialize_string(buf, pos, name, name_len);
    serialize_null(buf, pos);
    break;
  case IS_LONG:
    set_type(buf, pos, BSON_LONG);
    serialize_string(buf, pos, name, name_len);
    serialize_long(buf, pos, Z_LVAL_PP(data));
    break;
  case IS_DOUBLE:
    set_type(buf, pos, BSON_DOUBLE);
    serialize_string(buf, pos, name, name_len);
    serialize_double(buf, pos, Z_DVAL_PP(data));
    break;
  case IS_BOOL:
    set_type(buf, pos, BSON_BOOL);
    serialize_string(buf, pos, name, name_len);
    serialize_bool(buf, pos, Z_BVAL_PP(data));
    break;
  case IS_STRING: {
    set_type(buf, pos, BSON_STRING);
    serialize_string(buf, pos, name, name_len);

    long length = Z_STRLEN_PP(data);
    long length0 = length + BYTE_8;
    memcpy(buf+(*pos), &length0, INT_32);
    *(pos) = *pos + INT_32;

    serialize_string(buf, pos, Z_STRVAL_PP(data), length);
    break;
  }
  case IS_ARRAY: {
    set_type(buf, pos, BSON_OBJECT);
    serialize_string(buf, pos, name, name_len);

    zval_to_bson(buf, pos, Z_ARRVAL_PP(data) TSRMLS_CC);
  }
  case IS_OBJECT: {
    zend_class_entry *clazz = Z_OBJCE_PP( data );
    /* check for defined classes */
    // MongoId
    if(clazz == mongo_id_class) {
      set_type(buf, pos, BSON_OID);
      serialize_string(buf, pos, name, name_len);
      zval *zid = zend_read_property(mongo_id_class, *data, "id", 2, 0 TSRMLS_CC);
      serialize_string(buf, pos, Z_STRVAL_P(zid), OID_SIZE);
    }
    // MongoDate
    else if (clazz == mongo_date_class) {
      set_type(buf, pos, BSON_DATE);
      serialize_string(buf, pos, name, name_len);
      zval *zid = zend_read_property(mongo_date_class, *data, "ms", 2, 0 TSRMLS_CC);
      serialize_long(buf, pos, Z_LVAL_P(zid));
    }
    // MongoRegex
    else if (clazz == mongo_regex_class) {
      set_type(buf, pos, BSON_REGEX);
      serialize_string(buf, pos, name, name_len);
      zval *zid = zend_read_property(mongo_regex_class, *data, "regex", 5, 0 TSRMLS_CC);
      serialize_string(buf, pos, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
      zid = zend_read_property(mongo_regex_class, *data, "flags", 5, 0 TSRMLS_CC);
      serialize_string(buf, pos, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
    }
    // MongoCode
    else if (clazz == mongo_code_class) {
      set_type(buf, pos, BSON_CODE);
      serialize_string(buf, pos, name, name_len);

      // save spot for size
      int start = *pos;
      *(pos) += INT_32;
      zval *zid = zend_read_property(mongo_code_class, *data, "code", 4, 0 TSRMLS_CC);
      // string size
      serialize_int(buf, pos, Z_STRLEN_P(zid));
      // string
      serialize_string(buf, pos, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
      // scope
      zid = zend_read_property(mongo_code_class, *data, "scope", 5, 0 TSRMLS_CC);
      zval_to_bson(buf, pos, Z_ARRVAL_P(zid) TSRMLS_CC);

      // get total size
      serialize_size(buf, start, *pos);
    }
    // MongoBin
    else if (clazz == mongo_bindata_class) {
      set_type(buf, pos, BSON_BINARY);
      serialize_string(buf, pos, name, name_len);

      zval *zid = zend_read_property(mongo_bindata_class, *data, "length", 6, 0 TSRMLS_CC);
      serialize_int(buf, pos, Z_LVAL_P(zid));
      zid = zend_read_property(mongo_bindata_class, *data, "type", 4, 0 TSRMLS_CC);
      serialize_byte(buf, pos, (char)Z_LVAL_P(zid));
      zid = zend_read_property(mongo_bindata_class, *data, "bin", 3, 0 TSRMLS_CC);
      serialize_string(buf, pos, Z_STRVAL_P(zid), Z_STRLEN_P(zid));
    }
    break;
  }
  default:
    return FAILURE;
  }
  return SUCCESS;
}

int serialize_byte(char *buf, int *pos, char b) {
  buf[*pos] = b;
  return *(pos) = (*pos) + BYTE_8;
}

int serialize_string(char *buf, int *ppos, char *str, int str_len) {
  memcpy(buf+(*ppos), str, str_len);
  *(ppos) = *ppos + str_len;
  // add \0 at the end of the string
  buf[*ppos] = 0;
  return *(ppos) = *ppos + BYTE_8;
}

int serialize_int(char *buf, int *pos, int num) {
  memcpy(buf+(*pos), &num, INT_32);
  return *(pos) = (*pos) + INT_32;
}

int serialize_long(char *buf, int *ppos, long num) {
  memcpy(buf+(*ppos), &num, INT_64);
  return *(ppos) = (*ppos) + INT_64;
}

int serialize_double(char *buf, int *ppos, double num) {
  memcpy(buf+(*ppos), &num, DOUBLE_64);
  return *(ppos) = (*ppos) + DOUBLE_64;
}

/* the position is not increased, we are just filling
 * in the first 4 bytes with the size.
 */
int serialize_size(char *buf, int start, int end) {
  int total = end-start;
  memcpy(buf+start, &total, INT_32);
  return end;
}


char *bson_to_zval(char *buf, zval *result TSRMLS_DC) {

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
      bson_to_zval(buf, d TSRMLS_CC);
      add_assoc_zval(result, name, d);
      buf++;
      break;
    }
    case BSON_BINARY: {
      int len;
      memcpy(&len, buf, INT_32);
      buf += INT_32;

      char type = *buf++;

      char *bytes = (char*)emalloc(len);
      int i=0;
      for(i=0;i<len;i++) {
        *(bytes) = *buf++;
      }

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
    case BSON_LONG: {
      long d;
      memcpy(&d, buf, INT_64);
      add_assoc_long(result, name, d);
      buf += INT_64;
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
      int max_regex = 128;
      int max_flags = 16;
      char regex[max_regex];
      char flags[max_flags];

      int regex_len = 0;
      while (regex_len < max_regex && (regex[regex_len++] = *buf++));
      regex[regex_len] = 0;
      int flags_len = 0;
      while (flags_len < max_flags && (flags[flags_len++] = *buf++));
      flags[flags_len] = 0;

      zval *zegex;
      MAKE_STD_ZVAL(zegex);
      object_init_ex(zegex, mongo_regex_class);

      add_property_stringl(zegex, "regex", regex, regex_len, 1);
      add_property_stringl(zegex, "flags", flags, flags_len, 1);
      add_assoc_zval(result, name, zegex);

      break;
    }
    case BSON_CODE: 
    case BSON_CODE__D: 
    default: {
      php_printf("type %d not supported\n", type);
    }
    }
  }
  return buf;
}


int prep_obj_for_db(mongo::BSONObjBuilder *obj_builder, HashTable *php_array TSRMLS_DC) {
  zval **data;
  // check if "_id" field, 4 is length of "_id" + 1 for \0
  if (zend_hash_find(php_array, "_id", 4, (void**)&data) == FAILURE) {
    mongo::OID oid;
    oid.init();
    obj_builder->appendOID("_id", &oid);
    return 1;
  }
  add_to_bson(obj_builder, "_id", data TSRMLS_CC);
  return 0;
}

