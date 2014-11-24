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
#ifndef MONGO_DB_H
#define MONGO_DB_H

#define MONGO_COLLECTION_RETURN_TYPE_NAME       0
#define MONGO_COLLECTION_RETURN_TYPE_OBJECT     1
#define MONGO_COLLECTION_RETURN_TYPE_INFO_ARRAY 2

zend_object_value mongo_init_MongoDB_new(zend_class_entry* TSRMLS_DC);

/* Create a fake cursor that can be used to query the db from C. */
zval* mongo_db__create_fake_cursor(mongo_connection *connection, char *database, zval *cmd TSRMLS_DC);

/* Switch to primary connection */
void php_mongo_connection_force_primary(mongo_cursor *cursor);

int php_mongo_db_is_valid_dbname(char *dbname, int dbname_len TSRMLS_DC);
void php_mongo_db_construct(zval *z_client, zval *zlink, char *name, int name_len TSRMLS_DC);
zval *php_mongo_db_selectcollection(zval *z_client, char *collection, int collection_len TSRMLS_DC);

int php_mongodb_init(zval *zdb, zval *zlink, char *name, int name_len TSRMLS_DC);

/* Runs a MongoDB command.
 * NOTE: Exceptions are cleared, and the entire result document/error is returned.
 * On invalid database name or no servers available, returns NULL and raises an exception. */
zval *php_mongo_runcommand(zval *zmongoclient, mongo_read_preference *read_preferences, char *dbname, int dbname_len, zval *cmd, zval *options, int cursor_allowed, mongo_connection **used_connection TSRMLS_DC);

PHP_METHOD(MongoDB, __construct);
PHP_METHOD(MongoDB, __toString);
PHP_METHOD(MongoDB, __get);
PHP_METHOD(MongoDB, selectCollection);
PHP_METHOD(MongoDB, getGridFS);
PHP_METHOD(MongoDB, getSlaveOkay);
PHP_METHOD(MongoDB, setSlaveOkay);
PHP_METHOD(MongoDB, getReadPreference);
PHP_METHOD(MongoDB, setReadPreference);
PHP_METHOD(MongoDB, getProfilingLevel);
PHP_METHOD(MongoDB, setProfilingLevel);
PHP_METHOD(MongoDB, drop);
PHP_METHOD(MongoDB, repair);
PHP_METHOD(MongoDB, createCollection);
PHP_METHOD(MongoDB, dropCollection);
PHP_METHOD(MongoDB, listCollections);
PHP_METHOD(MongoDB, createDBRef);
PHP_METHOD(MongoDB, getDBRef);
PHP_METHOD(MongoDB, execute);
PHP_METHOD(MongoDB, command);
PHP_METHOD(MongoDB, lastError);
PHP_METHOD(MongoDB, prevError);
PHP_METHOD(MongoDB, resetError);
PHP_METHOD(MongoDB, forceError);
PHP_METHOD(MongoDB, authenticate);

#endif /* MONGO_DB_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
