/**
 *  Copyright 2009-2014 MongoDB, Inc.
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
#include <zend_exceptions.h>
#include "../php_mongo.h"
#include "id.h"
#include "../bson.h"

ZEND_EXTERN_MODULE_GLOBALS(mongo)

extern zend_class_entry *mongo_ce_Exception;

zend_class_entry *mongo_ce_Id = NULL;

zend_object_handlers mongo_id_handlers;

void generate_id(char *data TSRMLS_DC)
{
	int inc;

#ifdef WIN32
	int pid = GetCurrentThreadId();
#else
	int pid = (int)getpid();
#endif

	unsigned t = (unsigned) time(0);
	char *T = (char*)&t,
	*M = (char*)&MonGlo(machine),
	*P = (char*)&pid,
	*I = (char*)&inc;

	/* inc */
	inc = MonGlo(inc);
	MonGlo(inc)++;

	/* actually generate the MongoId */
#if PHP_C_BIGENDIAN
	/* 4 bytes ts */
	memcpy(data, T, 4);

	/* we add 1 or 2 to the pointers so we don't end up with all 0s, as the
	 * interesting stuff is at the end for big endian systems */

	/* 3 bytes machine */
	memcpy(data + 4, M + 1, 3);

	/* 2 bytes pid */
	memcpy(data + 7, P + 2, 2);

	/* 3 bytes inc */
	memcpy(data + 9, I + 1, 3);
#else
	/* 4 bytes ts */
	data[0] = T[3];
	data[1] = T[2];
	data[2] = T[1];
	data[3] = T[0];

	/* 3 bytes machine */
	memcpy(data + 4, M, 3);

	/* 2 bytes pid */
	memcpy(data + 7, P, 2);

	/* 3 bytes inc */
	data[9] = I[2];
	data[10] = I[1];
	data[11] = I[0];
#endif
}

static int php_mongo_is_valid_id(const char *id)
{
	if (id == NULL) {
		return 0;
	}

	if (strlen(id) != 24) {
		return 0;
	}

	if (strspn(id, "0123456789abcdefABCDEF") != 24) {
		return 0;
	}

	return 1;
}

int php_mongo_compare_ids(zval *o1, zval *o2 TSRMLS_DC)
{
	if (
		Z_TYPE_P(o1) == IS_OBJECT && Z_TYPE_P(o2) == IS_OBJECT &&
		instanceof_function(Z_OBJCE_P(o1), mongo_ce_Id TSRMLS_CC) &&
		instanceof_function(Z_OBJCE_P(o2), mongo_ce_Id TSRMLS_CC)
	) {
		int i;

		mongo_id *id1 = (mongo_id*)zend_object_store_get_object(o1 TSRMLS_CC);
		mongo_id *id2 = (mongo_id*)zend_object_store_get_object(o2 TSRMLS_CC);

		for (i=0; i<12; i++) {
			if (id1->id[i] < id2->id[i]) {
				return -1;
			} else if (id1->id[i] > id2->id[i]) {
				return 1;
			}
		}
		return 0;
	}

	return 1;
}

static void php_mongo_id_free(void *object TSRMLS_DC)
{
	mongo_id *id = (mongo_id*)object;

	if (id) {
		if (id->id) {
			efree(id->id);
		}
		zend_object_std_dtor(&id->std TSRMLS_CC);
		efree(id);
	}
}

static zend_object_value php_mongo_id_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	mongo_id *intern;

	intern = (mongo_id*)emalloc(sizeof(mongo_id));
	memset(intern, 0, sizeof(mongo_id));

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	init_properties(intern);

	retval.handle = zend_objects_store_put(intern,
	(zend_objects_store_dtor_t) zend_objects_destroy_object,
	php_mongo_id_free, NULL TSRMLS_CC);
	retval.handlers = &mongo_id_handlers;

	return retval;
}

/* Converts a 12-byte ObjectId buffer into a hexadecimal string. This function
 * expects id_str to be a valid buffer >= 12 bytes. The returned string will be
 * 25 bytes (24 hex characters + null byte) and must be freed by the caller. */
char *php_mongo_mongoid_to_hex(const char *id_str)
{
	int i;
	char *id = (char*)emalloc(25);

	for ( i = 0; i < 12; i++) {
		int x = *id_str;
		char digit1, digit2;

		if (*id_str < 0) {
			x = 256 + *id_str;
		}

		digit1 = x / 16;
		digit2 = x % 16;

		id[2 * i]   = (digit1 < 10) ? '0' + digit1 : digit1 - 10 + 'a';
		id[2 * i + 1] = (digit2 < 10) ? '0' + digit2 : digit2 - 10 + 'a';

		id_str++;
	}

	id[24] = '\0';

	return id;
}

/* {{{ MongoId::__toString()
 */
PHP_METHOD(MongoId, __toString)
{
	mongo_id *this_id;
	char *id;

	this_id = (mongo_id*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED_STRING(this_id->id, MongoId);

	id = php_mongo_mongoid_to_hex(this_id->id);

	RETURN_STRING(id, NO_DUP);
}
/* }}} */

/* {{{ MongoId::__construct()
 */
PHP_METHOD(MongoId, __construct)
{
	zval *id = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &id) == FAILURE) {
		return;
	}

	php_mongo_mongoid_populate(getThis(), id TSRMLS_CC);
}
/* }}} */

void php_mongo_mongoid_populate(zval *this_ptr, zval *id TSRMLS_DC)
{
	mongo_id *this_id = (mongo_id*)zend_object_store_get_object(this_ptr TSRMLS_CC);

	if (!this_id->id) {
		this_id->id = (char*)emalloc(OID_SIZE + 1);
		this_id->id[OID_SIZE] = '\0';
	}

	if (id && Z_TYPE_P(id) == IS_STRING && Z_STRLEN_P(id) == 24) {
		int i;

		if (!php_mongo_is_valid_id(Z_STRVAL_P(id))) {
			zend_throw_exception(mongo_ce_Exception, "ID must be valid hex characters", 18 TSRMLS_CC);
			return;
		}
		for (i = 0; i < 12;i++) {
			char digit1 = Z_STRVAL_P(id)[i * 2], digit2 = Z_STRVAL_P(id)[i * 2 + 1];

			digit1 = digit1 >= 'a' && digit1 <= 'f' ? digit1 - 87 : digit1;
			digit1 = digit1 >= 'A' && digit1 <= 'F' ? digit1 - 55 : digit1;
			digit1 = digit1 >= '0' && digit1 <= '9' ? digit1 - 48 : digit1;

			digit2 = digit2 >= 'a' && digit2 <= 'f' ? digit2 - 87 : digit2;
			digit2 = digit2 >= 'A' && digit2 <= 'F' ? digit2 - 55 : digit2;
			digit2 = digit2 >= '0' && digit2 <= '9' ? digit2 - 48 : digit2;

			this_id->id[i] = digit1 * 16 + digit2;
		}

		zend_update_property(mongo_ce_Id, this_ptr, "$id", strlen("$id"), id TSRMLS_CC);
	} else if (id && Z_TYPE_P(id) == IS_OBJECT && Z_OBJCE_P(id) == mongo_ce_Id) {
		zval *str;

		mongo_id *that_id = (mongo_id*)zend_object_store_get_object(id TSRMLS_CC);

		memcpy(this_id->id, that_id->id, OID_SIZE);

		str = zend_read_property(mongo_ce_Id, id, "$id", strlen("$id"), NOISY TSRMLS_CC);
		zend_update_property(mongo_ce_Id, this_ptr, "$id", strlen("$id"), str TSRMLS_CC);
	} else if (id) {
		zend_throw_exception(mongo_ce_Exception, "Invalid object ID", 19 TSRMLS_CC);
		return;
	} else {
		zval *str = NULL;
		char *tmp_id;
		generate_id(this_id->id TSRMLS_CC);

		MAKE_STD_ZVAL(str);

		tmp_id = php_mongo_mongoid_to_hex(this_id->id);
		ZVAL_STRING(str, tmp_id, 0);
		zend_update_property(mongo_ce_Id, this_ptr, "$id", strlen("$id"), str TSRMLS_CC);
		zval_ptr_dtor(&str);
	}
}

/* {{{ MongoId::__set_state()
 */
PHP_METHOD(MongoId, __set_state)
{
	zval *state, **id;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &state) == FAILURE) {
		return;
	}

	if (zend_hash_find(HASH_P(state), "$id", strlen("$id") + 1, (void**) &id) == FAILURE) {
		return;
	}

	object_init_ex(return_value, mongo_ce_Id);
	php_mongo_mongoid_populate(return_value, *id TSRMLS_CC);
}
/* }}} */

/* {{{ MongoId::getTimestamp
 */
PHP_METHOD(MongoId, getTimestamp)
{
	int ts = 0, i;
	mongo_id *id = (mongo_id*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED_STRING(id->id, MongoId);

	for ( i = 0; i < 4; i++) {
		int x = ((int)id->id[i] < 0) ? 256 + id->id[i] : id->id[i];
		ts = (ts * 256) + x;
	}

	RETURN_LONG(ts);
}
/* }}} */

/* {{{ MongoId::getPID
 */
PHP_METHOD(MongoId, getPID)
{
	int pid = 0, i;
	mongo_id *id = (mongo_id*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED_STRING(id->id, MongoId);

	for (i = 8; i > 6; i--) {
		int x;

		x = ((int)id->id[i] < 0) ? 256 + id->id[i] : id->id[i];
		pid = (pid * 256) + x;
	}

	RETURN_LONG(pid);
}
/* }}} */

PHP_METHOD(MongoId, getInc)
{
	int inc = 0;
	char *ptr = (char*)&inc;
	mongo_id *id = (mongo_id*)zend_object_store_get_object(getThis() TSRMLS_CC);
	MONGO_CHECK_INITIALIZED_STRING(id->id, MongoId);

	/* 11, 10, 9, '\0' */
	ptr[0] = id->id[11];
	ptr[1] = id->id[10];
	ptr[2] = id->id[9];

	RETURN_LONG(inc);
}

/* {{{ MongoId::getHostname
 */
PHP_METHOD(MongoId, getHostname)
{
	char hostname[256];

	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	RETURN_STRING(hostname, 1);
}
/* }}} */

/* {{{ proto static bool MongoID::isValid(mixed id)
   Returns true if $id is valid MongoID, false on failure */
PHP_METHOD(MongoId, isValid)
{
	zval **mongoid;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "Z", &mongoid) == FAILURE) {
		return;
	}

	switch (Z_TYPE_PP(mongoid)) {
		case IS_OBJECT:
			if (instanceof_function(Z_OBJCE_PP(mongoid), mongo_ce_Id TSRMLS_CC)) {
				RETURN_TRUE;
			}
			RETURN_FALSE;
			break;

		case IS_LONG:
			convert_to_string_ex(mongoid);
			/* break intentionally omitted */

		case IS_STRING:
			if (php_mongo_is_valid_id(Z_STRVAL_PP(mongoid))) {
				RETURN_TRUE;
			}
			RETURN_FALSE;
			break;
	}

	RETURN_FALSE;
}
/* }}} */


int php_mongo_id_serialize(zval *struc, unsigned char **serialized_data, zend_uint *serialized_length, zend_serialize_data *var_hash TSRMLS_DC)
{
	char *tmp_id;
	mongo_id *this_id;

	this_id = (mongo_id*)zend_object_store_get_object(struc TSRMLS_CC);
	tmp_id = php_mongo_mongoid_to_hex(this_id->id);
	*(serialized_length) = strlen(tmp_id);
	*(serialized_data) = (unsigned char*)tmp_id;

	return SUCCESS;
}

int php_mongo_id_unserialize(zval **rval, zend_class_entry *ce, const unsigned char* p, zend_uint datalen, zend_unserialize_data* var_hash TSRMLS_DC)
{
	zval *str;

	MAKE_STD_ZVAL(str);
	ZVAL_STRINGL(str, (const char*)p, 24, 1);

	object_init_ex(*rval, mongo_ce_Id);

	php_mongo_mongoid_populate(*rval, str TSRMLS_CC);
	zval_ptr_dtor(&str);

	return SUCCESS;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_isValid, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, id)
ZEND_END_ARG_INFO()

static zend_function_entry MongoId_methods[] = {
	PHP_ME(MongoId, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoId, __toString, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoId, __set_state, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(MongoId, getTimestamp, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoId, getHostname, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(MongoId, getPID, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoId, getInc, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoId, isValid, arginfo_isValid, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)

	PHP_FE_END
};

void mongo_init_MongoId(TSRMLS_D)
{
	zend_class_entry id;
	INIT_CLASS_ENTRY(id, "MongoId", MongoId_methods);

	id.create_object = php_mongo_id_new;
	id.serialize = php_mongo_id_serialize;
	id.unserialize = php_mongo_id_unserialize;

	mongo_ce_Id = zend_register_internal_class(&id TSRMLS_CC);

	zend_declare_property_null(mongo_ce_Id, "$id", strlen("$id"), ZEND_ACC_PUBLIC|MONGO_ACC_READ_ONLY TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
