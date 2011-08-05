// hash.c
/**
 *  Copyright 2009-2011 10gen, Inc.
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
#include "hash.h"

/**
 * Removes objects and arrays (called before copying hashes)
 */
static int remove_objects(void *pDest TSRMLS_DC);

void mongo_util_hash_copy_to_p(void *pDest) {
  zval **p = (zval**)pDest;
  zval *temp = *p;

  *p = (zval*)malloc(sizeof(zval));
  memcpy(*p, temp, sizeof(zval));
  INIT_PZVAL(*p);

  switch (Z_TYPE_PP(p)) {
  case IS_STRING: {
    Z_STRVAL_PP(p) = (char*)malloc(Z_STRLEN_P(temp)+1);
    memcpy(Z_STRVAL_PP(p), Z_STRVAL_P(temp), Z_STRLEN_P(temp)+1);
    break;
  }
  case IS_ARRAY: {
    TSRMLS_FETCH();
    mongo_util_hash_to_pzval(p, &temp TSRMLS_CC);
    break;
  }
  }
}

void mongo_util_hash_copy_to_np(void *pDest) {
  zval **p = (zval**)pDest;
  zval *temp = *p;

  ALLOC_ZVAL(*p);
  memcpy(*p, temp, sizeof(zval));
  INIT_PZVAL(*p);

  switch(Z_TYPE_PP(p)) {
  case IS_STRING: {
    Z_STRVAL_PP(p) = estrndup(Z_STRVAL_P(temp), Z_STRLEN_P(temp));
    break;
  }
  case IS_ARRAY: {
    TSRMLS_FETCH();
    mongo_util_hash_to_zval(p, &temp TSRMLS_CC);
    break;
  }
  }
}

int mongo_util_hash_to_pzval(zval** destination, zval** source TSRMLS_DC) {
  HashTable *ht;
  zval temp, *dest;

  dest = (zval*)malloc(sizeof(zval));
  ht = (HashTable*)malloc(sizeof(HashTable));
  if (!dest || !ht) {
    return FAILURE;
  }

  // remove timestamp objs from source
  zend_hash_apply(Z_ARRVAL_PP(source), remove_objects TSRMLS_CC);

  zend_hash_init(ht, 8, 0, mongo_util_hash_dtor, 1);
  zend_hash_copy(ht, Z_ARRVAL_PP(source), (copy_ctor_func_t)mongo_util_hash_copy_to_p, &temp, sizeof(zval*));

  INIT_PZVAL(dest);
  Z_TYPE_P(dest) = IS_ARRAY;
  Z_ARRVAL_P(dest) = ht;

  *destination = dest;
  return SUCCESS;
}

int mongo_util_hash_to_zval(zval** destination, zval** source TSRMLS_DC) {
  HashTable *ht;
  zval temp, *dest;

  ALLOC_ZVAL(dest);
  ALLOC_HASHTABLE(ht);

  zend_hash_init(ht, 8, 0, ZVAL_PTR_DTOR, 0);
  zend_hash_copy(ht, Z_ARRVAL_PP(source), (copy_ctor_func_t)mongo_util_hash_copy_to_np, &temp, sizeof(zval*));

  INIT_PZVAL(dest);
  Z_TYPE_P(dest) = IS_ARRAY;
  Z_ARRVAL_P(dest) = ht;

  *destination = dest;
  return SUCCESS;
}

static int remove_objects(void *pDest TSRMLS_DC) {
  switch (Z_TYPE_PP((zval**)pDest)) {
  case IS_OBJECT:
  case IS_ARRAY:
      return ZEND_HASH_APPLY_REMOVE;
  }
  return ZEND_HASH_APPLY_KEEP;
}

void mongo_util_hash_dtor(void *pDest) {
  zval **elem = (zval**)pDest;
  switch (Z_TYPE_PP(elem)) {
  case IS_NULL:
  case IS_LONG:
  case IS_DOUBLE:
  case IS_BOOL:
  case IS_STRING:
    zval_internal_dtor(*elem);
    break;
  case IS_ARRAY:
    zend_hash_destroy(Z_ARRVAL_PP(elem));
    free(Z_ARRVAL_PP(elem));
    break;
  }
  free(*elem);
}

