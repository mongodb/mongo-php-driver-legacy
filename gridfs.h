//gridfs.h
/**
 *  Copyright 2009-2010 10gen, Inc.
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

#ifndef MONGO_GRIDFS_H
#define MONGO_GRIDFS_H

PHP_METHOD(MongoGridFS, __construct);
PHP_METHOD(MongoGridFS, drop);
PHP_METHOD(MongoGridFS, find);
PHP_METHOD(MongoGridFS, storeFile);
PHP_METHOD(MongoGridFS, findOne);
PHP_METHOD(MongoGridFS, remove);
PHP_METHOD(MongoGridFS, storeUpload);
PHP_METHOD(MongoGridFS, storeBytes);
PHP_METHOD(MongoGridFS, get);
PHP_METHOD(MongoGridFS, put);
PHP_METHOD(MongoGridFS, delete);

PHP_METHOD(MongoGridFSFile, __construct);
PHP_METHOD(MongoGridFSFile, getFilename);
PHP_METHOD(MongoGridFSFile, getSize);
PHP_METHOD(MongoGridFSFile, write);
PHP_METHOD(MongoGridFSFile, getBytes);

PHP_METHOD(MongoGridFSCursor, __construct);
PHP_METHOD(MongoGridFSCursor, getNext);
PHP_METHOD(MongoGridFSCursor, current);
PHP_METHOD(MongoGridFSCursor, key);

#endif /* MONGO_GRIDFS_H */
