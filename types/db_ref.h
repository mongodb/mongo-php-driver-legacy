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
#ifndef __TYPES_DB_REF_H__
#define __TYPES_DB_REF_H__

/* Creates a DBRef array('$id' => zid, '$ref' => ns [, '$db' => db])
 * Note that zid needs to be a zval as $id can be a string/int/float
 * If an array/object is passed in we'll try to locate _id key and use that
 */
zval *php_mongo_dbref_create(zval *zid, char *ns, char *db);

PHP_METHOD(MongoDBRef, create);
PHP_METHOD(MongoDBRef, isRef);
PHP_METHOD(MongoDBRef, get);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
