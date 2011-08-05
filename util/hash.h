// hash.h
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

#ifndef MONGO_UTIL_HASH_H
#define MONGO_UTIL_HASH_H

/**
 * copy a non-persistent zval** to a persistent zval**
 */
void mongo_util_hash_copy_to_p(void *pDest);

/**
 * copy a persistent zval** to a non-persistent zval**
 */
void mongo_util_hash_copy_to_np(void *pDest);

/**
 * Copy a non-persistent zval hash to a persistent zval hash
 */
int mongo_util_hash_to_pzval(zval **dest, zval **source TSRMLS_DC);

/**
 * Copy a persistent zval hash to a non-persistent zval hash
 */
int mongo_util_hash_to_zval(zval **dest, zval **source TSRMLS_DC);

void mongo_util_hash_dtor(void *pDest);

#endif
