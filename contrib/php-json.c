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
  | Author: Omar Kilani <omar@php.net>                                   |
  +----------------------------------------------------------------------+
*/

#include "php-json.h"

/*
  php_mongo_is_numeric_array() is borrowed, slightly modifieid, from ext/json
  json_determine_array_type()
*/
int php_mongo_is_numeric_array(zval *val TSRMLS_DC)
{
	int i;
	HashTable *myht = HASH_OF(val);

	i = myht ? zend_hash_num_elements(myht) : 0;
	if (i > 0) {
		char *key;
		ulong index, idx;
		uint key_len;
		HashPosition pos;

		zend_hash_internal_pointer_reset_ex(myht, &pos);
		idx = 0;
		for (;; zend_hash_move_forward_ex(myht, &pos)) {
			i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);
			if (i == HASH_KEY_NON_EXISTANT) {
				break;
			}

			if (i == HASH_KEY_IS_STRING) {
				return FAILURE;
			} else {
				if (index != idx) {
					return FAILURE;
				}
			}
			idx++;
		}
	}

	return SUCCESS;
}
