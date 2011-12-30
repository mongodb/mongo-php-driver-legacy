// log.c
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

#include "../php_mongo.h"
#include "log.h"

zend_class_entry *mongo_ce_Log;
ZEND_EXTERN_MODULE_GLOBALS(mongo);

static long set_value(char *setting, zval *return_value TSRMLS_DC);
static void get_value(char *setting, zval *return_value TSRMLS_DC);

static zend_function_entry mongo_log_methods[] = {
  PHP_ME(MongoLog, setLevel, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoLog, getLevel, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoLog, setModule, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(MongoLog, getModule, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  {NULL, NULL, NULL}
};

void mongo_init_MongoLog(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoLog", mongo_log_methods);
  mongo_ce_Log = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_Log, "NONE", strlen("NONE"), MONGO_LOG_NONE TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_Log, "WARNING", strlen("WARNING"), MONGO_LOG_WARNING TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Log, "INFO", strlen("INFO"), MONGO_LOG_INFO TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Log, "FINE", strlen("FINE"), MONGO_LOG_FINE TSRMLS_CC);

  zend_declare_class_constant_long(mongo_ce_Log, "RS", strlen("RS"), MONGO_LOG_RS TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Log, "POOL", strlen("POOL"), MONGO_LOG_POOL TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Log, "IO", strlen("IO"), MONGO_LOG_IO TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Log, "SERVER", strlen("SERVER"), MONGO_LOG_SERVER TSRMLS_CC);
  zend_declare_class_constant_long(mongo_ce_Log, "ALL", strlen("ALL"), MONGO_LOG_ALL TSRMLS_CC);

  zend_declare_property_long(mongo_ce_Log, "level", strlen("level"), 0, ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
  zend_declare_property_long(mongo_ce_Log, "module", strlen("module"), 0, ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
}

static long set_value(char *setting, zval *return_value TSRMLS_DC) {
  long value;

  if (zend_parse_parameters(1 TSRMLS_CC, "l", &value) == FAILURE) {
    return 0;
  }

  zend_update_static_property_long(mongo_ce_Log, setting, strlen(setting), value TSRMLS_CC);

  return value;
}

static void get_value(char *setting, zval *return_value TSRMLS_DC) {
  zval *value;

  value = zend_read_static_property(mongo_ce_Log, setting, strlen(setting), NOISY TSRMLS_CC);

  ZVAL_LONG(return_value, Z_LVAL_P(value));
}

PHP_METHOD(MongoLog, setLevel)
{
	MonGlo(log_level) = set_value("level", return_value TSRMLS_CC);
}

PHP_METHOD(MongoLog, getLevel)
{
	get_value("level", return_value TSRMLS_CC);
}

PHP_METHOD(MongoLog, setModule)
{
	MonGlo(log_module) = set_value("module", return_value TSRMLS_CC);
}

PHP_METHOD(MongoLog, getModule)
{
	get_value("module", return_value TSRMLS_CC);
}

void mongo_log(const int module, const int level TSRMLS_DC, const char *format, ...)
{
	if ((module & MonGlo(log_module)) && (level & MonGlo(log_level))) {
		va_list args;

		va_start(args, format);
		php_verror(NULL, "", E_NOTICE, format, args TSRMLS_CC);
		va_end(args);
	}
}
