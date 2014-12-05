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
#ifndef __TYPES_DB_REF_H__
#define __TYPES_DB_REF_H__

/* Creates a DBRef array of the form:
 *
 *   array('$ref' => collection, '$id' => zid, [, '$db' => db])
 *
 * See: http://docs.mongodb.org/manual/reference/database-references/#dbrefs
 */
zval *php_mongo_dbref_create(zval *zid, char *collection, char *db TSRMLS_DC);

void php_mongo_dbref_get(zval *zdb, zval *ref, zval *return_value TSRMLS_DC);

/* Resolves a document or ID parameter provided to the collection/db createDBRef
 * methods. If the argument is an array or object, we'll attempt to return its
 * _id field, otherwise, the argument is returned as-is.
 *
 * NULL will be returned if, and only if, the argument is an array without an
 * _id field. This is legacy behavior, which should probably be changed in the
 * future.
 */
zval *php_mongo_dbref_resolve_id(zval *zid TSRMLS_DC);

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
