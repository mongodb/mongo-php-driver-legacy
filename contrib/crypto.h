/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef MONGO_CONTRIB_CRYPTO_H
#define MONGO_CONTRIB_CRYPTO_H

#include "php.h"

typedef void (*php_mongo_hash_init_func_t)(void *context);
typedef void (*php_mongo_hash_update_func_t)(void *context, const unsigned char *buf, unsigned int count);
typedef void (*php_mongo_hash_final_func_t)(unsigned char *digest, void *context);
typedef int  (*php_mongo_hash_copy_func_t)(const void *ops, void *orig_context, void *dest_context);


typedef struct _php_mongo_hash_ops {
	php_mongo_hash_init_func_t hash_init;
	php_mongo_hash_update_func_t hash_update;
	php_mongo_hash_final_func_t hash_final;
	php_mongo_hash_copy_func_t hash_copy;

	int digest_size;
	int block_size;
	int context_size;
} php_mongo_hash_ops;

void php_mongo_sha1(const unsigned char *data, int data_len, unsigned char *return_value);
void php_mongo_hmac(unsigned char *data, int data_len, char *key, int key_len, unsigned char *return_value, int *return_value_len);
int php_mongo_hash_pbkdf2_sha1(char *password, int password_len, unsigned char *salt, int salt_len, long iterations, unsigned char *return_value, long *return_value_len TSRMLS_DC);

#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
