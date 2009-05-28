// util.c
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

#include "mongo.h"
#include "cursor.h"

extern zend_class_entry *spl_ce_InvalidArgumentException,
  *mongo_ce_Cursor;

extern int le_connection,
  le_pconnection;


zend_class_entry *mongo_ce_Util = NULL;

static char *replace_dots(char *key, int key_len, char *position) {
  int i;
  for (i=0; i<key_len; i++) {
    if (key[i] == '.') {
      *(position)++ = '_';
    }
    else {
      *(position)++ = key[i];
    }
  }
  return position;
}

/* {{{ MongoUtil::toIndexString(array|string) */
PHP_METHOD(MongoUtil, toIndexString) {
  zval *zkeys;
  int param_count = 1;
  char *name, *position;
  int len = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zkeys) == FAILURE) {
    RETURN_FALSE;
  }

  if (Z_TYPE_P(zkeys) == IS_ARRAY) {
    HashTable *hindex = Z_ARRVAL_P(zkeys);
    HashPosition pointer;
    zval **data;
    char *key;
    uint key_len, first = 1, key_type;
    ulong index;

    for(zend_hash_internal_pointer_reset_ex(hindex, &pointer); 
        zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS; 
        zend_hash_move_forward_ex(hindex, &pointer)) {

      key_type = zend_hash_get_current_key_ex(hindex, &key, &key_len, &index, NO_DUP, &pointer);
      switch (key_type) {
      case HASH_KEY_IS_STRING: {
        len += key_len;

        convert_to_long(*data);
        len += Z_LVAL_PP(data) == 1 ? 2 : 3;

        break;
      }
      case HASH_KEY_IS_LONG:
        convert_to_string(*data);

        len += Z_STRLEN_PP(data);
        len += 2;
        break;
      default:
        continue;
      }
    }

    name = (char*)emalloc(len+1);
    position = name;

    for(zend_hash_internal_pointer_reset_ex(hindex, &pointer); 
        zend_hash_get_current_data_ex(hindex, (void**)&data, &pointer) == SUCCESS; 
        zend_hash_move_forward_ex(hindex, &pointer)) {

      if (!first) {
        *(position)++ = '_';
      }
      first = 0;

      key_type = zend_hash_get_current_key_ex(hindex, &key, &key_len, &index, NO_DUP, &pointer);

      if (key_type == HASH_KEY_IS_LONG) {
        key_len = spprintf(&key, 0, "%ld", index);
        key_len += 1;
      }

      // copy str, replacing '.' with '_'
      position = replace_dots(key, key_len-1, position);
      
      *(position)++ = '_';
      
      convert_to_long(*data);
      if (Z_LVAL_PP(data) != 1) {
        *(position)++ = '-';
      }
      *(position)++ = '1';

      if (key_type == HASH_KEY_IS_LONG) {
        efree(key);
      }
    }
    *(position) = 0;
  }
  else {
    int len;
    convert_to_string(zkeys);

    len = Z_STRLEN_P(zkeys);

    name = (char*)emalloc(len + 3);
    position = name;
 
    // copy str, replacing '.' with '_'
    position = replace_dots(Z_STRVAL_P(zkeys), Z_STRLEN_P(zkeys), position);

    *(position)++ = '_';
    *(position)++ = '1';
    *(position) = '\0';
  }
  RETURN_STRING(name, 0)
}



static function_entry MongoUtil_methods[] = {
  PHP_ME(MongoUtil, toIndexString, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  { NULL, NULL, NULL }
};


void mongo_init_MongoUtil(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoUtil", MongoUtil_methods);
  mongo_ce_Util = zend_register_internal_class(&ce TSRMLS_CC);


  zend_declare_class_constant_string(mongo_ce_Util, "LT", strlen("LT"), "$lt" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "LTE", strlen("LTE"), "$lte" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "GT", strlen("GT"), "$gt" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "GTE", strlen("GTE"), "$gte" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "IN", strlen("IN"), "$in" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "NE", strlen("NE"), "$ne" TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_Util, "ASC", strlen("ASC"), 1 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Util, "DESC", strlen("DESC"), -1 TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_Util, "BIN_FUNCTION", strlen("BIN_FUNCTION"), 1 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Util, "BIN_ARRAY", strlen("BIN_ARRAY"), 2 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Util, "BIN_UUID", strlen("BIN_UUID"), 3 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Util, "BIN_MD5", strlen("BIN_MD5"), 5 TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Util, "BIN_CUSTOM", strlen("BIN_CUSTOM"), 128 TSRMLS_CC);

  zend_declare_class_constant_string(mongo_ce_Util, "ADMIN", strlen("ADMIN"), "admin" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "AUTHENTICATE", strlen("AUTHENTICATE"), "authenticate" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "CREATE_COLLECTION", strlen("CREATE_COLLECTION"), "create" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "DELETE_INDICES", strlen("DELETE_INDICES"), "deleteIndexes" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "DROP", strlen("DROP"), "drop" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "DROP_DATABASE", strlen("DROP_DATABASE"), "dropDatabase" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "FORCE_ERROR", strlen("FORCE_ERROR"), "forceerror" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "INDEX_INFO", strlen("INDEX_INFO"), "cursorInfo" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "LAST_ERROR", strlen("LAST_ERROR"), "getlasterror" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "LIST_DATABASES", strlen("LIST_DATABASES"), "listDatabases" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "LOGGING", strlen("LOGGING"), "opLogging" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "LOGOUT", strlen("LOGOUT"), "logout" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "NONCE", strlen("NONCE"), "getnonce" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "PREV_ERROR", strlen("PREV_ERROR"), "getpreverror" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "PROFILE", strlen("PROFILE"), "profile" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "QUERY_TRACING", strlen("QUERY_TRACING"), "queryTraceLevel" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "REPAIR_DATABASE", strlen("REPAIR_DATABASE"), "repairDatabase" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "RESET_ERROR", strlen("RESET_ERROR"), "reseterror" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "SHUTDOWN", strlen("SHUTDOWN"), "shutdown" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "TRACING", strlen("TRACING"), "traceAll" TSRMLS_CC);
  zend_declare_class_constant_string(mongo_ce_Util, "VALIDATE", strlen("VALIDATE"), "validate" TSRMLS_CC);
}
