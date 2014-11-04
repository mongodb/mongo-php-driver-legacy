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

#include <contrib/crypto.h>
#include "ext/standard/sha1.h"

int php_hash_copy(const void *ops, void *orig_context, void *dest_context) /* {{{ */
{
	php_hash_ops *hash_ops = (php_hash_ops *)ops;

	memcpy(dest_context, orig_context, hash_ops->context_size);
	return SUCCESS;
}
/* }}} */

const php_hash_ops sha1_hash_ops = {
	(php_hash_init_func_t) PHP_SHA1Init,
	(php_hash_update_func_t) PHP_SHA1Update,
	(php_hash_final_func_t) PHP_SHA1Final,
	(php_hash_copy_func_t) php_hash_copy,
	20,
	64,
	sizeof(PHP_SHA1_CTX)
};

static inline void php_hash_string_xor_char(unsigned char *out, const unsigned char *in, const unsigned char xor_with, const int length) {
	int i;
	for (i=0; i < length; i++) {
		out[i] = in[i] ^ xor_with;
	}
}

static inline void php_hash_string_xor(unsigned char *out, const unsigned char *in, const unsigned char *xor_with, const int length) {
	int i;
	for (i=0; i < length; i++) {
		out[i] = in[i] ^ xor_with[i];
	}
}

static inline void php_hash_hmac_prep_key(unsigned char *K, const php_hash_ops *ops, void *context, const unsigned char *key, const int key_len) {
	memset(K, 0, ops->block_size);
	if (key_len > ops->block_size) {
		/* Reduce the key first */
		ops->hash_init(context);
		ops->hash_update(context, key, key_len);
		ops->hash_final(K, context);
	} else {
		memcpy(K, key, key_len);
	}
	/* XOR the key with 0x36 to get the ipad) */
	php_hash_string_xor_char(K, K, 0x36, ops->block_size);
}

static inline void php_hash_hmac_round(unsigned char *final, const php_hash_ops *ops, void *context, const unsigned char *key, const unsigned char *data, const long data_size) {
	ops->hash_init(context);
	ops->hash_update(context, key, ops->block_size);
	ops->hash_update(context, data, data_size);
	ops->hash_final(final, context);
}






static void php_hash_do_hash_hmac(INTERNAL_FUNCTION_PARAMETERS, int isfilename, zend_bool raw_output_default) /* {{{ */
{
	char *algo, *data, *digest, *key, *K;
	int algo_len, data_len, key_len;
	zend_bool raw_output = raw_output_default;
	const php_hash_ops *ops;
	void *context;
	php_stream *stream = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|b", &algo, &algo_len, &data, &data_len, 
																  &key, &key_len, &raw_output) == FAILURE) {
		return;
	}

	ops = php_hash_fetch_ops(algo, algo_len);
	if (!ops) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown hashing algorithm: %s", algo);
		RETURN_FALSE;
	}
	if (isfilename) {
		stream = php_stream_open_wrapper_ex(data, "rb", REPORT_ERRORS, NULL, DEFAULT_CONTEXT);
		if (!stream) {
			/* Stream will report errors opening file */
			RETURN_FALSE;
		}
	}

	context = emalloc(ops->context_size);

	K = emalloc(ops->block_size);
	digest = emalloc(ops->digest_size + 1);

	php_hash_hmac_prep_key((unsigned char *) K, ops, context, (unsigned char *) key, key_len);		

	if (isfilename) {
		char buf[1024];
		int n;
		ops->hash_init(context);
		ops->hash_update(context, (unsigned char *) K, ops->block_size);
		while ((n = php_stream_read(stream, buf, sizeof(buf))) > 0) {
			ops->hash_update(context, (unsigned char *) buf, n);
		}
		php_stream_close(stream);
		ops->hash_final((unsigned char *) digest, context);
	} else {
		php_hash_hmac_round((unsigned char *) digest, ops, context, (unsigned char *) K, (unsigned char *) data, data_len);
	}

	php_hash_string_xor_char((unsigned char *) K, (unsigned char *) K, 0x6A, ops->block_size);

	php_hash_hmac_round((unsigned char *) digest, ops, context, (unsigned char *) K, (unsigned char *) digest, ops->digest_size);

	/* Zero the key */
	memset(K, 0, ops->block_size);
	efree(K);
	efree(context);

	if (raw_output) {
		digest[ops->digest_size] = 0;
		RETURN_STRINGL(digest, ops->digest_size, 0);
	} else {
		char *hex_digest = safe_emalloc(ops->digest_size, 2, 1);

		php_hash_bin2hex(hex_digest, (unsigned char *) digest, ops->digest_size);
		hex_digest[2 * ops->digest_size] = 0;
		efree(digest);
		RETURN_STRINGL(hex_digest, 2 * ops->digest_size, 0);
	}
}
/* }}} */
/* {{{ proto string hash_pbkdf2(string algo, string password, string salt, int iterations [, int length = 0, bool raw_output = false])
Generate a PBKDF2 hash of the given password and salt
Returns lowercase hexits by default */
PHP_FUNCTION(hash_pbkdf2)
{
	char *returnval, *algo, *salt, *pass;
	unsigned char *computed_salt, *digest, *temp, *result, *K1, *K2;
	long loops, i, j, iterations, length = 0, digest_length;
	int algo_len, pass_len, salt_len;
	zend_bool raw_output = 0;
	const php_hash_ops *ops;
	void *context;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sssl|lb", &algo, &algo_len, &pass, &pass_len, &salt, &salt_len, &iterations, &length, &raw_output) == FAILURE) {
		return;
	}

	ops = php_hash_fetch_ops(algo, algo_len);
	if (!ops) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown hashing algorithm: %s", algo);
		RETURN_FALSE;
	}

	if (iterations <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Iterations must be a positive integer: %ld", iterations);
		RETURN_FALSE;
	}

	if (length < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Length must be greater than or equal to 0: %ld", length);
		RETURN_FALSE;
	}

	if (salt_len > INT_MAX - 4) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Supplied salt is too long, max of INT_MAX - 4 bytes: %d supplied", salt_len);
		RETURN_FALSE;
	}

	context = emalloc(ops->context_size);
	ops->hash_init(context);

	K1 = emalloc(ops->block_size);
	K2 = emalloc(ops->block_size);
	digest = emalloc(ops->digest_size);
	temp = emalloc(ops->digest_size);

	/* Setup Keys that will be used for all hmac rounds */
	php_hash_hmac_prep_key(K1, ops, context, (unsigned char *) pass, pass_len);
	/* Convert K1 to opad -- 0x6A = 0x36 ^ 0x5C */
	php_hash_string_xor_char(K2, K1, 0x6A, ops->block_size);

	/* Setup Main Loop to build a long enough result */
	if (length == 0) {
		length = ops->digest_size;
		if (!raw_output) {
			length = length * 2;
		}
	}
	digest_length = length;
	if (!raw_output) {
		digest_length = (long) ceil((float) length / 2.0);
	}

	loops = (long) ceil((float) digest_length / (float) ops->digest_size);

	result = safe_emalloc(loops, ops->digest_size, 0);

	computed_salt = safe_emalloc(salt_len, 1, 4);
	memcpy(computed_salt, (unsigned char *) salt, salt_len);

	for (i = 1; i <= loops; i++) {
		/* digest = hash_hmac(salt + pack('N', i), password) { */

		/* pack("N", i) */
		computed_salt[salt_len] = (unsigned char) (i >> 24);
		computed_salt[salt_len + 1] = (unsigned char) ((i & 0xFF0000) >> 16);
		computed_salt[salt_len + 2] = (unsigned char) ((i & 0xFF00) >> 8);
		computed_salt[salt_len + 3] = (unsigned char) (i & 0xFF);

		php_hash_hmac_round(digest, ops, context, K1, computed_salt, (long) salt_len + 4);
		php_hash_hmac_round(digest, ops, context, K2, digest, ops->digest_size);
		/* } */

		/* temp = digest */
		memcpy(temp, digest, ops->digest_size);

		/* 
		 * Note that the loop starting at 1 is intentional, since we've already done
		 * the first round of the algorithm.
		 */
		for (j = 1; j < iterations; j++) {
			/* digest = hash_hmac(digest, password) { */
			php_hash_hmac_round(digest, ops, context, K1, digest, ops->digest_size);
			php_hash_hmac_round(digest, ops, context, K2, digest, ops->digest_size);
			/* } */
			/* temp ^= digest */
			php_hash_string_xor(temp, temp, digest, ops->digest_size);
		}
		/* result += temp */
		memcpy(result + ((i - 1) * ops->digest_size), temp, ops->digest_size);
	}
	/* Zero potentially sensitive variables */
	memset(K1, 0, ops->block_size);
	memset(K2, 0, ops->block_size);
	memset(computed_salt, 0, salt_len + 4);
	efree(K1);
	efree(K2);
	efree(computed_salt);
	efree(context);
	efree(digest);
	efree(temp);

	returnval = safe_emalloc(length, 1, 1);
	if (raw_output) {
		memcpy(returnval, result, length);
	} else {
		php_hash_bin2hex(returnval, result, digest_length);
	}
	returnval[length] = 0;
	efree(result);
	RETURN_STRINGL(returnval, length, 0);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
