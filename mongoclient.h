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

#ifndef MONGOCLIENT_H
#define MONGOCLIENT_H

int php_mongo_create_le(mongo_cursor *cursor, char *name TSRMLS_DC);

void mongo_init_MongoClient(TSRMLS_D);

/*
 * Mongo class
 */
PHP_METHOD(MongoClient, __construct);
PHP_METHOD(MongoClient, getConnections);
PHP_METHOD(MongoClient, connect);
PHP_METHOD(MongoClient, pairConnect);
PHP_METHOD(MongoClient, persistConnect);
PHP_METHOD(MongoClient, pairPersistConnect);
PHP_METHOD(MongoClient, connectUtil);
PHP_METHOD(MongoClient, __toString);
PHP_METHOD(MongoClient, __get);
PHP_METHOD(MongoClient, selectDB);
PHP_METHOD(MongoClient, selectCollection);
PHP_METHOD(MongoClient, getSlaveOkay);
PHP_METHOD(MongoClient, setSlaveOkay);
PHP_METHOD(MongoClient, getReadPreference);
PHP_METHOD(MongoClient, setReadPreference);
PHP_METHOD(MongoClient, dropDB);
PHP_METHOD(MongoClient, lastError);
PHP_METHOD(MongoClient, prevError);
PHP_METHOD(MongoClient, resetError);
PHP_METHOD(MongoClient, forceError);
PHP_METHOD(MongoClient, close);
PHP_METHOD(MongoClient, listDBs);
PHP_METHOD(MongoClient, getHosts);
PHP_METHOD(MongoClient, getSlave);
PHP_METHOD(MongoClient, switchSlave);

#endif

