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
  default:
    return FAILURE;
  }
  return SUCCESS;
}

int set_byte(char *buf, int *pos, char b) {
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


int php_array_to_bson( mongo::BSONObjBuilder *obj_builder, HashTable *arr_hash TSRMLS_DC) {
  zval **data;
  char *key;
  uint key_len;
  ulong index;
  zend_bool duplicate = 0;
  HashPosition pointer;
  int num = 0;

  if (zend_hash_num_elements(arr_hash) == 0) {
    return num;
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

    add_to_bson(obj_builder, field_name, data TSRMLS_CC);
    efree(field_name);
  }

  return num;
}

void bson_to_php_array(mongo::BSONObj *obj, zval *array TSRMLS_DC) {
  mongo::BSONObjIterator it = mongo::BSONObjIterator(*obj);
  while (it.more()) {
    mongo::BSONElement elem = it.next();

    char *key = (char*)elem.fieldName();

    switch( elem.type() ) {
    case mongo::Undefined:
    case mongo::jstNULL: {
      add_assoc_null(array, key);
      break;
    }
    case mongo::NumberInt: {
      long num = (long)elem.number();
      add_assoc_long(array, key, num);
      break;
    }
    case mongo::NumberDouble: {
      double num = elem.number();
      add_assoc_double(array, key, num);
      break;
    }
    case mongo::Bool: {
      int b = elem.boolean();
      add_assoc_bool(array, key, b);
      break;
    }
    case mongo::String: {
      char *value = (char*)elem.valuestr();
      add_assoc_string(array, key, value, 1);
      break;
    }
    case mongo::Date: {
      zval *zdate = bson_to_zval_date(elem.date() TSRMLS_CC);
      add_assoc_zval(array, key, zdate);
      break;
    }
    case mongo::RegEx: {
      zval *zre = bson_to_zval_regex((char*)elem.regex(), (char*)elem.regexFlags() TSRMLS_CC);
      add_assoc_zval(array, key, zre);
      break;
    }
    case mongo::BinData: {
      int size;
      char *bin = (char*)elem.binData(size);
      int type = elem.binDataType();
      zval *phpbin = bson_to_zval_bin(bin, size, type TSRMLS_CC);
      add_assoc_zval(array, key, phpbin);
      break;
    }
    case mongo::Code: {
      zval *empty;
      ALLOC_INIT_ZVAL(empty);
      array_init(empty);

      zval *zode = bson_to_zval_code(elem.valuestr(), empty TSRMLS_CC);
      add_assoc_zval(array, key, zode);
      break;
    }
    case mongo::CodeWScope: {
      mongo::BSONObj bscope = elem.codeWScopeObject();
      zval *scope;

      ALLOC_INIT_ZVAL(scope);
      array_init(scope);
      bson_to_php_array(&bscope, scope TSRMLS_CC);

      zval *zode = bson_to_zval_code(elem.codeWScopeCode(), scope TSRMLS_CC);

      // get rid of extra reference to scope
      zval_ptr_dtor(&scope);

      add_assoc_zval(array, key, zode);
      break;
    }
    case mongo::Array:
    case mongo::Object: {
      mongo::BSONObj temp = elem.embeddedObject();
      zval *subarray;
      ALLOC_INIT_ZVAL(subarray);
      array_init(subarray);
      bson_to_php_array(&temp, subarray TSRMLS_CC);
      add_assoc_zval(array, key, subarray);
      break;
    }
    case mongo::jstOID: {
      zval *zoid = bson_to_zval_oid(elem.__oid() TSRMLS_CC);
      add_assoc_zval(array, key, zoid);
      break;
    }
    case mongo::EOO: {
      break;
    }
    default:
      zend_error(E_WARNING, "bson=>php: type %i not supported\n", elem.type());
    }
  }
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

int add_to_bson(mongo::BSONObjBuilder *obj_builder, char *field_name, zval **data TSRMLS_DC) {
  switch (Z_TYPE_PP(data)) {
  case IS_NULL:
    obj_builder->appendNull(field_name);
    break;
  case IS_LONG:
    obj_builder->append(field_name, (int)Z_LVAL_PP(data));
    break;
  case IS_DOUBLE:
    obj_builder->append(field_name, Z_DVAL_PP(data));
    break;
  case IS_BOOL:
    obj_builder->append(field_name, Z_BVAL_PP(data));
    break;
  case IS_CONSTANT_ARRAY:
  case IS_ARRAY: {
    mongo::BSONObjBuilder subobj;
    php_array_to_bson(&subobj, Z_ARRVAL_PP(data) TSRMLS_CC);
    obj_builder->append(field_name, subobj.done());
    break;
  }
  case IS_STRING: {
    string s(Z_STRVAL_PP(data), Z_STRLEN_PP(data));
    obj_builder->append(field_name, s);
    break;
  }
  case IS_OBJECT: {
    TSRMLS_FETCH();
    zend_class_entry *clazz = Z_OBJCE_PP( data );
    /* check for defined classes */
    // MongoId
    if(clazz == mongo_id_class) {
      mongo::OID oid;
      zval_to_bson_oid(data, &oid TSRMLS_CC);
      obj_builder->appendOID(field_name, &oid); 
    }
    // MongoDate
    else if (clazz == mongo_date_class) {
      obj_builder->appendDate(field_name, zval_to_bson_date(data TSRMLS_CC)); 
    }
    else if (clazz == mongo_regex_class) {
      char *re, *flags;
      zval_to_bson_regex(data, &re, &flags TSRMLS_CC);
      obj_builder->appendRegex(field_name, re, flags); 
    }
    // MongoCode
    else if (clazz == mongo_code_class) {
      char *code;
      mongo::BSONObjBuilder scope;
      zval_to_bson_code(data, &code, &scope TSRMLS_CC);
      // add code & scope to obj_builder
      obj_builder->appendCodeWScope(field_name, code, scope.done()); 

    }
    else if (clazz == mongo_bindata_class) {
      char *bin;
      int len;
      switch(zval_to_bson_bin(data, &bin, &len TSRMLS_CC)) {
      case 1:
        obj_builder->appendBinData(field_name, len, mongo::Function, bin); 
        break;
      case 3:
        obj_builder->appendBinData(field_name, len, mongo::bdtUUID, bin); 
        break;
      case 5:
        obj_builder->appendBinData(field_name, len, mongo::MD5Type, bin); 
        break;
      case 128:
        obj_builder->appendBinData(field_name, len, mongo::bdtCustom, bin); 
        break;
      default:
        obj_builder->appendBinData(field_name, len, mongo::ByteArray, bin); 
        break;
      }
    }
    break;
  }
  case IS_RESOURCE:
  case IS_CONSTANT:
  default:
    return FAILURE;
  }
  return SUCCESS;
}
