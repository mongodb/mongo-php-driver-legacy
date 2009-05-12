//gridfs.c
/**
 *  Copyright 2009 10gen, Inc.
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

#include "collection.h"
#include "cursor.h"
#include "bson.h"
#include "mongo.h"
#include "mongo_types.h"

extern zend_class_entry *mongo_ce_Collection,
  *mongo_ce_Cursor;

zend_class_entry *mongo_ce_GridFS = NULL,
  *mongo_ce_GridFSFile = NULL,
  *mongo_ce_GridFSCursor = NULL;

PHP_METHOD(MongoGridFS, __construct) {
  zval *zdb;
  char *files = 0, *chunks = 0;
  int files_len = 0, chunks_len = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &zdb, mongo_ce_DB, &files, &files_len, &chunks, &chunks_len) == FAILURE) {
    return;
  }

  if (!files && !chunks) {
    zend_update_property_string(mongo_ce_GridFS, getThis(), "filesName", strlen("filesName"), "fs.files" TSRMLS_CC);
    zend_update_property_string(mongo_ce_GridFS, getThis(), "chunksName", strlen("chunksName"), "fs.chunks" TSRMLS_CC);
  }
  else if (!chunks) {
    char *files_name, *chunks_name;
    spprintf(&files_name, 0, "%s.files", files);
    spprintf(&chunks_name, 0, "%s.chunks", files);
    zend_update_property_string(mongo_ce_GridFS, getThis(), "filesName", strlen("filesName"), files_name TSRMLS_CC);
    zend_update_property_string(mongo_ce_GridFS, getThis(), "chunksName", strlen("chunksName"), chunks_name TSRMLS_CC);
    efree(files_name);
    efree(chunks_name);
  }
  else {
    zend_update_property_string(mongo_ce_GridFS, getThis(), "filesName", strlen("filesName"), files TSRMLS_CC);
    zend_update_property_string(mongo_ce_GridFS, getThis(), "chunksName", strlen("chunksName"), chunks TSRMLS_CC);
  }

  zval *zfile;
  MAKE_STD_ZVAL(zfile);
  ZVAL_STRING(zfile, files, 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 4, zdb, zfile, 2, NULL);
  zim_MongoCollection___construct(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);

  zval *zchunks;
  MAKE_STD_ZVAL(zchunks);
  object_init_ex(zchunks, mongo_ce_Collection);

  zval *zchunk;
  MAKE_STD_ZVAL(zchunk);
  ZVAL_STRING(zchunk, chunks, 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 4, zdb, zchunk, 2, NULL);
  zim_MongoCollection___construct(2, return_value, return_value_ptr, zchunks, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
  
  zend_update_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunksName"), zchunks TSRMLS_CC);

  // ensure index on chunks.n
  zval *zidx;
  MAKE_STD_ZVAL(zidx);
  ZVAL_STRING(zidx, "n", 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, zidx, 1, NULL);
  zim_MongoCollection_ensureIndex(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zend_update_property(mongo_ce_GridFS, getThis(), "db", strlen("db"), zdb TSRMLS_CC);
}


PHP_METHOD(MongoGridFS, drop) {
  zval *zchunks = zend_read_property(mongo_ce_Mongo, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);
  zval *zdb = zend_read_property(mongo_ce_Mongo, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, zchunks, 1, NULL);
  zim_MongoDB_dropCollection(1, return_value, return_value_ptr, zdb, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zim_MongoCollection_drop(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
}

PHP_METHOD(MongoGridFS, find) {
}

static function_entry MongoGridFS_methods[] = {
  PHP_ME(MongoGridFS, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, drop, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, find, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, storeFile, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, findOne, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, remove, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, storeUpload, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

void mongo_init_MongoGridFS(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoGridFS", MongoGridFS_methods);
  mongo_ce_GridFS = zend_register_internal_class_ex(&ce, mongo_ce_Collection, "MongoCollection" TSRMLS_CC);

  zend_declare_property_null(mongo_ce_GridFS, "resource", strlen("resource"), ZEND_ACC_PUBLIC TSRMLS_CC);
  zend_declare_property_null(mongo_ce_GridFS, "chunks", strlen("chunks"), ZEND_ACC_PUBLIC TSRMLS_CC);

  zend_declare_property_null(mongo_ce_GridFS, "filesName", strlen("filesName"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_GridFS, "chunksName", strlen("chunksName"), ZEND_ACC_PROTECTED TSRMLS_CC);
}


static function_entry MongoGridFSFile_methods[] = {
  PHP_ME(MongoGridFSFile, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFSFile, getFilename, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFSFile, getSize, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFSFile, write, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFSFile, getBytes, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

void mongo_init_MongoGridFSFile(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoGridFSFile", MongoGridFSFile_methods);
  mongo_ce_GridFSFile = zend_register_internal_class(&ce TSRMLS_CC);

  zend_declare_property_null(mongo_ce_GridFSFile, "file", strlen("file"), ZEND_ACC_PUBLIC TSRMLS_CC);

  zend_declare_property_null(mongo_ce_GridFSFile, "gridfs", strlen("gridfs"), ZEND_ACC_PROTECTED TSRMLS_CC);
}


static function_entry MongoGridFSCursor_methods[] = {
  PHP_ME(MongoGridFSCursor, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFSCursor, getNext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFSCursor, current, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFSCursor, key, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

void mongo_init_MongoGridFSCursor(TSRMLS_D) {
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MongoGridFSCursor", MongoGridFSCursor_methods);
  mongo_ce_GridFSCursor = zend_register_internal_class_ex(&ce, mongo_ce_Cursor, "MongoCursor" TSRMLS_CC);
}
