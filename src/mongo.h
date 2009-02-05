
#ifndef PHP_MONGO_H
#define PHP_MONGO_H 1

#define PHP_MONGO_VERSION "1.0"
#define PHP_MONGO_EXTNAME "mongo"

#define PHP_DB_CLIENT_CONNECTION_RES_NAME "mongo connection"

PHP_MINIT_FUNCTION(mongo);
PHP_MSHUTDOWN_FUNCTION(mongo);

PHP_FUNCTION(mongo___construct);
PHP_FUNCTION(mongo_connect);
PHP_FUNCTION(mongo_close);
PHP_FUNCTION(mongo_set_db);
PHP_FUNCTION(mongo_get_db);
PHP_FUNCTION(mongo_ensure_index);
PHP_FUNCTION(mongo_find);
PHP_FUNCTION(mongo_find_one);
PHP_FUNCTION(mongo_has_next);
PHP_FUNCTION(mongo_kill_cursors);
PHP_FUNCTION(mongo_next);
PHP_FUNCTION(mongo_remove);
PHP_FUNCTION(mongo_insert);
PHP_FUNCTION(mongo_update);
PHP_FUNCTION(mongo_limit);
PHP_FUNCTION(mongo_sort);

PHP_FUNCTION(temp);

extern zend_module_entry mongo_module_entry;
#define phpext_mongo_ptr &mongo_module_entry

#endif
