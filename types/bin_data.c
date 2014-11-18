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
#include "bin_data.h"

extern zend_class_entry *mongo_ce_Exception;

zend_class_entry *mongo_ce_BinData = NULL;

/* {{{ MongoBinData::__construct()
 */
PHP_METHOD(MongoBinData, __construct)
{
	char *bin;
	int bin_len;
	long type = PHP_MONGO_BIN_GENERIC;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &bin, &bin_len, &type) == FAILURE) {
		return;
	}

	if (type == PHP_MONGO_BIN_UUID_RFC4122 && bin_len != PHP_MONGO_BIN_UUID_RFC4122_SIZE) {
		zend_throw_exception_ex(mongo_ce_Exception, 25 TSRMLS_CC, "RFC4122 UUID must be %d bytes; actually: %d", PHP_MONGO_BIN_UUID_RFC4122_SIZE, bin_len);
		return;
	}

	zend_update_property_stringl(mongo_ce_BinData, getThis(), "bin", strlen("bin"), bin, bin_len TSRMLS_CC);
	zend_update_property_long(mongo_ce_BinData, getThis(), "type", strlen("type"), type TSRMLS_CC);
}
/* }}} */


/* {{{ MongoBinData::__toString()
 */
PHP_METHOD(MongoBinData, __toString)
{
	RETURN_STRING( "<Mongo Binary Data>", 1 );
}
/* }}} */


static zend_function_entry MongoBinData_methods[] = {
	PHP_ME(MongoBinData, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoBinData, __toString, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void mongo_init_MongoBinData(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoBinData", MongoBinData_methods);
	ce.create_object = php_mongo_type_object_new;
	mongo_ce_BinData = zend_register_internal_class(&ce TSRMLS_CC);

	/* fields */
	zend_declare_property_string(mongo_ce_BinData, "bin", strlen("bin"), "", ZEND_ACC_PUBLIC|MONGO_ACC_READ_ONLY TSRMLS_CC);
	zend_declare_property_long(mongo_ce_BinData, "type", strlen("type"), 0, ZEND_ACC_PUBLIC|MONGO_ACC_READ_ONLY TSRMLS_CC);

	/* constants */
	zend_declare_class_constant_long(mongo_ce_BinData, "GENERIC", strlen("GENERIC"), PHP_MONGO_BIN_GENERIC TSRMLS_CC);
	/* can't use FUNCTION because it's a reserved word */
	zend_declare_class_constant_long(mongo_ce_BinData, "FUNC", strlen("FUNC"), PHP_MONGO_BIN_FUNC TSRMLS_CC);
	/* can't use ARRAY because it's a reserved word */
	zend_declare_class_constant_long(mongo_ce_BinData, "BYTE_ARRAY", strlen("BYTE_ARRAY"), PHP_MONGO_BIN_BYTE_ARRAY TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_BinData, "UUID", strlen("UUID"), PHP_MONGO_BIN_UUID TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_BinData, "UUID_RFC4122", strlen("UUID_RFC4122"), PHP_MONGO_BIN_UUID_RFC4122 TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_BinData, "MD5", strlen("MD5"), PHP_MONGO_BIN_MD5 TSRMLS_CC);
	zend_declare_class_constant_long(mongo_ce_BinData, "CUSTOM", strlen("CUSTOM"), PHP_MONGO_BIN_CUSTOM TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
