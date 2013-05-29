/**
 *  Copyright 2009-2013 10gen, Inc.
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
#include "../php_mongo.h"
#include "date.h"

zend_class_entry *mongo_ce_Date = NULL;

void php_mongo_mongodate_make_now(long *sec, long *usec)
{
#ifdef WIN32
	*sec = (long) time(0);
#else
	struct timeval time;

	gettimeofday(&time, NULL);
	*sec = time.tv_sec;
	*usec = (time.tv_usec / 1000) * 1000;
#endif
}

/* {{{ MongoDate::__construct
 */
PHP_METHOD(MongoDate, __construct)
{
	long sec = 0, usec = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll", &sec, &usec) == FAILURE) {
		return;
	}

	if (ZEND_NUM_ARGS() == 0) {
		php_mongo_mongodate_make_now(&sec, &usec);
	}

	php_mongo_mongodate_populate(getThis(), sec, usec TSRMLS_CC);
}
/* }}} */

void php_mongo_mongodate_populate(zval *mongocode_object, long sec, long usec TSRMLS_DC)
{
	zend_update_property_long(mongo_ce_Date, mongocode_object, "usec", strlen("usec"), usec TSRMLS_CC);
	zend_update_property_long(mongo_ce_Date, mongocode_object, "sec", strlen("sec"), sec TSRMLS_CC);
}


/* {{{ MongoDate::__toString()
 */
PHP_METHOD(MongoDate, __toString)
{
	zval *zsec = zend_read_property( mongo_ce_Date, getThis(), "sec", strlen("sec"), NOISY TSRMLS_CC );
	long sec = Z_LVAL_P( zsec );
	zval *zusec = zend_read_property( mongo_ce_Date, getThis(), "usec", strlen("usec"), NOISY TSRMLS_CC );
	long usec = Z_LVAL_P( zusec );
	double dusec = (double)usec/1000000;
	char *str;

	spprintf(&str, 0, "%.8f %ld", dusec, sec);
	RETURN_STRING(str, 0);
}
/* }}} */


static zend_function_entry MongoDate_methods[] = {
	PHP_ME(MongoDate, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDate, __toString, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

void mongo_init_MongoDate(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoDate", MongoDate_methods);
	ce.create_object = php_mongo_type_object_new;
	mongo_ce_Date = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_property_long(mongo_ce_Date, "sec", strlen("sec"), 0, ZEND_ACC_PUBLIC|MONGO_ACC_READ_ONLY TSRMLS_CC);
	zend_declare_property_long(mongo_ce_Date, "usec", strlen("usec"), 0, ZEND_ACC_PUBLIC|MONGO_ACC_READ_ONLY TSRMLS_CC);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
