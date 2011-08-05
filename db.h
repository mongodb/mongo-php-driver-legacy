// db.h
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


#ifndef MONGO_DB_H
#define MONGO_DB_H

zend_object_value mongo_init_MongoDB_new(zend_class_entry* TSRMLS_DC);

/**
 * Create a fake cursor that can be used to query the db from C.
 */
zval* mongo_db__create_fake_cursor(mongo_server *current, zval *cmd TSRMLS_DC);

/**
 * Run the given database command on the given server.
 */
zval* mongo_db_cmd(mongo_server *current, zval *cmd TSRMLS_DC);


PHP_METHOD(MongoDB, __construct);
PHP_METHOD(MongoDB, __toString);
PHP_METHOD(MongoDB, __get);
PHP_METHOD(MongoDB, selectCollection);
PHP_METHOD(MongoDB, getGridFS);
PHP_METHOD(MongoDB, getSlaveOkay);
PHP_METHOD(MongoDB, setSlaveOkay);
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
