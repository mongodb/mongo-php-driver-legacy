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
#include "../php_mongo.h"
#include "date.h"

zend_class_entry *mongo_ce_Date = NULL;
zend_object_handlers mongo_date_handlers;

typedef struct {
	zend_object std;
	int64_t     datetime;
} mongo_date;

void php_mongo_date_init(zval *value, int64_t datetime TSRMLS_DC)
{
	long        sec, usec;

	usec = (long) ((((datetime * 1000) % 1000000) + 1000000) % 1000000);
	sec  = (long) ((datetime/1000) - (datetime < 0 && usec));

	php_mongo_mongodate_populate(value, sec, usec TSRMLS_CC);
}

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

void php_mongo_mongodate_populate(zval *mongodate_object, long sec, long usec TSRMLS_DC)
{
	mongo_date *date;
	int64_t internal_date = 0;

	internal_date += usec / 1000;
	internal_date += (sec * 1000);

	usec = (usec / 1000) * 1000;
	
	zend_update_property_long(mongo_ce_Date, mongodate_object, "usec", strlen("usec"), usec TSRMLS_CC);
	zend_update_property_long(mongo_ce_Date, mongodate_object, "sec", strlen("sec"), sec TSRMLS_CC);

	date = (mongo_date*) zend_object_store_get_object(mongodate_object TSRMLS_CC);
	date->datetime = internal_date;
}

/* {{{ mongo_date::__construct
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

/* {{{ MongoDate::__toString()
 */
PHP_METHOD(MongoDate, __toString)
{
	mongo_date *date;
	int64_t     sec;
	int64_t     usec;
	double      dusec;
	char       *str;

	date = (mongo_date*) zend_object_store_get_object(getThis() TSRMLS_CC);

	usec  = (int64_t) ((((date->datetime * 1000) % 1000000) + 1000000) % 1000000);
	sec   = (int64_t) ((date->datetime/1000) - (date->datetime < 0 && usec));
	dusec = (double) usec / 1000000;

#ifdef WIN32
	spprintf(&str, 0, "%.8f %I64d", dusec, (int64_t) sec);
#else
	spprintf(&str, 0, "%.8f %lld", dusec, (long long int) sec);
#endif

	RETURN_STRING(str, 0);
}
/* }}} */


static zend_function_entry MongoDate_methods[] = {
	PHP_ME(MongoDate, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(MongoDate, __toString, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/* {{{ php_mongo_date_free
 */
static void php_mongo_date_free(void *object TSRMLS_DC)
{
	mongo_date *date = (mongo_date*)object;

	zend_object_std_dtor(&date->std TSRMLS_CC);

	efree(date);
}
/* }}} */

static zend_object_value php_mongodate_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	mongo_date *intern;

	intern = (mongo_date*)emalloc(sizeof(mongo_date));
	memset(intern, 0, sizeof(mongo_date));

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	init_properties(intern);

	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, php_mongo_date_free, NULL TSRMLS_CC);
	retval.handlers = &mongo_date_handlers;

	return retval;
}

void mongo_init_MongoDate(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MongoDate", MongoDate_methods);
	ce.create_object = php_mongodate_new;
	mongo_ce_Date = zend_register_internal_class(&ce TSRMLS_CC);
	memcpy(&mongo_date_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mongo_date_handlers.write_property = mongo_write_property;
	mongo_date_handlers.read_property = mongo_read_property;

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
