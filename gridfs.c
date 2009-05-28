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
#include <zend_exceptions.h>

#include "gridfs.h"
#include "collection.h"
#include "cursor.h"
#include "mongo.h"
#include "mongo_types.h"
#include "db.h"

extern zend_class_entry *mongo_ce_DB,
  *mongo_ce_Collection,
  *mongo_ce_Cursor,
  *mongo_ce_GridFSException,
  *mongo_ce_Id;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_GridFS = NULL,
  *mongo_ce_GridFSFile = NULL,
  *mongo_ce_GridFSCursor = NULL;

PHP_METHOD(MongoGridFS, __construct) {
  zval *zdb, *files = 0, *chunks = 0, *zchunks, *zidx;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|zz", &zdb, mongo_ce_DB, &files, &chunks) == FAILURE) {
    return;
  }

  if (!files && !chunks) {
    MAKE_STD_ZVAL(files);
    ZVAL_STRING(files, "fs.files", 1);
    MAKE_STD_ZVAL(chunks);
    ZVAL_STRING(chunks, "fs.chunks", 1);
  }
  else if (!chunks) {
    zval *temp_file;
    char *temp;

    MAKE_STD_ZVAL(chunks);
    spprintf(&temp, 0, "%s.chunks", Z_STRVAL_P(files));
    ZVAL_STRING(chunks, temp, 0);

    MAKE_STD_ZVAL(temp_file);
    spprintf(&temp, 0, "%s.files", Z_STRVAL_P(files));
    ZVAL_STRING(temp_file, temp, 0);
    files = temp_file;
  }
  else {
    convert_to_string(files);
    zval_add_ref(&files);
    convert_to_string(chunks);
    zval_add_ref(&chunks);
  }

  // create files collection
  PUSH_PARAM(zdb); PUSH_PARAM(files); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_MongoCollection___construct(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); 

  // create chunks collection
  MAKE_STD_ZVAL(zchunks);
  object_init_ex(zchunks, mongo_ce_Collection);

  PUSH_PARAM(zdb); PUSH_PARAM(chunks); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_MongoCollection___construct(2, return_value, return_value_ptr, zchunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); 
  
  // store
  zend_update_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), zchunks TSRMLS_CC);
  
  // ensure index on chunks.n
  MAKE_STD_ZVAL(zidx);
  array_init(zidx);
  add_assoc_long(zidx, "n", 1);

  PUSH_PARAM(zidx); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCollection_ensureIndex(1, return_value, return_value_ptr, zchunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  // cleanup
  zval_ptr_dtor(&zchunks);
  zval_ptr_dtor(&zidx);

  zval_ptr_dtor(&files);
  zval_ptr_dtor(&chunks);
}


PHP_METHOD(MongoGridFS, drop) {
  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  zval *zchunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  PUSH_PARAM(zchunks); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoDB_dropCollection(1, return_value, return_value_ptr, c->parent, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zim_MongoCollection_drop(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
}

PHP_METHOD(MongoGridFS, find) {
  zval temp;
  zval *zquery = 0, *zfields = 0;
  mongo_collection *c;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|aa", &zquery, &zfields) == FAILURE) {
    return;
  }

  if (!zquery) {
    MAKE_STD_ZVAL(zquery);
    array_init(zquery);
  }
  else {
    zval_add_ref(&zquery);
  }

  if (!zfields) {
    MAKE_STD_ZVAL(zfields);
    array_init(zfields);
  }
  else {
    zval_add_ref(&zquery);
  }

  object_init_ex(return_value, mongo_ce_GridFSCursor);

  c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);

  PUSH_PARAM(getThis()); PUSH_PARAM(c->db->link); PUSH_PARAM(c->ns); PUSH_PARAM(zquery); PUSH_PARAM(zfields); PUSH_PARAM((void*)5);
  PUSH_EO_PARAM();
  zim_MongoGridFSCursor___construct(5, &temp, NULL, return_value, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&zquery);
  zval_ptr_dtor(&zfields);
}

PHP_METHOD(MongoGridFS, storeFile) {
  char *filename;
  int filename_len, chunk_num = 0, chunk_size;
  zval temp;
  zval *extra = 0, *zid, *zfile, *chunks;
  FILE *fp;
  long size, pos = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &filename, &filename_len, &extra) == FAILURE) {
    return;
  }

  // try to open the file
  fp = fopen(filename, "rb");
  if (!fp) {
    zend_throw_exception_ex(mongo_ce_GridFSException, 0 TSRMLS_CC, "could not open file %s", filename);
    return;
  }

  // get size
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  if (size >= 0xffffffff) {
    zend_throw_exception_ex(mongo_ce_GridFSException, 0 TSRMLS_CC, "file %s is too large: %ld bytes", filename, size);
    return;
  }


  // create an id for the file
  MAKE_STD_ZVAL(zid);
  object_init_ex(zid, mongo_ce_Id);
  zim_MongoId___construct(0, &temp, NULL, zid, return_value_used TSRMLS_CC);

  MAKE_STD_ZVAL(zfile);
  array_init(zfile);
  
  // reset file ptr
  fseek(fp, 0, SEEK_SET);

  // insert chunks
  chunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);
  while (pos < size) {
    char *buf;
    zval *zchunk;

    chunk_size = size-pos >= MonGlo(chunk_size) ? MonGlo(chunk_size) : size-pos;
    buf = (char*)emalloc(chunk_size);
    if (fread(buf, 1, chunk_size, fp) < chunk_size) {
      zend_throw_exception_ex(mongo_ce_GridFSException, 0 TSRMLS_CC, "error reading file %s", filename);
      return;
    }

    // create chunk
    MAKE_STD_ZVAL(zchunk);
    array_init(zchunk);

    add_assoc_zval(zchunk, "files_id", zid);
    add_assoc_long(zchunk, "n", chunk_num);
    add_assoc_stringl(zchunk, "data", buf, chunk_size, DUP);

    // insert chunk

    PUSH_PARAM(zchunk); PUSH_PARAM((void*)1);
    PUSH_EO_PARAM();
    zim_MongoCollection_insert(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
    POP_EO_PARAM();
    POP_PARAM(); POP_PARAM(); 
    
    // increment counters
    pos += chunk_size;
    chunk_num++; 

    zval_add_ref(&zid); // zid->refcount = 2
    zval_ptr_dtor(&zchunk); // zid->refcount = 1
    efree(buf);
  }
  // close file ptr
  fclose(fp);

  add_assoc_zval(zfile, "_id", zid);
  add_assoc_stringl(zfile, "filename", filename, filename_len, DUP);
  add_assoc_long(zfile, "length", size);
  add_assoc_long(zfile, "chunkSize", MonGlo(chunk_size));

  if (extra) {
    zval temp;
    zend_hash_merge(Z_ARRVAL_P(zfile), Z_ARRVAL_P(extra), NULL, &temp, sizeof(zval), 1);
  }

  // insert file
  PUSH_PARAM(zfile); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCollection_insert(1, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); 

  // cleanup
  zend_objects_store_del_ref(zid TSRMLS_CC);
  zval_ptr_dtor(&zid);
  zval_ptr_dtor(&zfile);
}

PHP_METHOD(MongoGridFS, findOne) {
  zval *zquery = 0, *file;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &zquery) == FAILURE) {
    return;
  }

  if (!zquery) {
    MAKE_STD_ZVAL(zquery);
    array_init(zquery);
  }
  else if (Z_TYPE_P(zquery) != IS_ARRAY) {
    zval *temp;

    convert_to_string(zquery);

    MAKE_STD_ZVAL(temp);
    array_init(temp);
    add_assoc_string(temp, "filename", Z_STRVAL_P(zquery), 1);

    zquery = temp;
  }
  else {
    zval_add_ref(&zquery);
  }

  MAKE_STD_ZVAL(file);

  PUSH_PARAM(zquery); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCollection_findOne(1, file, &file, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); 

  if (Z_TYPE_P(file) == IS_NULL) {
    RETVAL_ZVAL(file, 0, 1);
  }
  else {
    zval temp;

    object_init_ex(return_value, mongo_ce_GridFSFile);

    PUSH_PARAM(getThis()); PUSH_PARAM(file); PUSH_PARAM((void*)2);
    PUSH_EO_PARAM();
    zim_MongoGridFSFile___construct(2, &temp, NULL, return_value, return_value_used TSRMLS_CC);
    POP_EO_PARAM();
    POP_PARAM(); POP_PARAM(); POP_PARAM();
  }

  zval_ptr_dtor(&file);
  zval_ptr_dtor(&zquery);
}


PHP_METHOD(MongoGridFS, remove) {
  zval zjust_one;
  zval *criteria = 0, *zfields, *zcursor, *chunks, *next;
  zend_bool just_one = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ab", &criteria, &just_one) == FAILURE) {
    return;
  }

  if (!criteria) {
    MAKE_STD_ZVAL(criteria);
    array_init(criteria);
  }
  else {
    zval_add_ref(&criteria);
  }

  MAKE_STD_ZVAL(zfields);
  array_init(zfields);
  add_assoc_long(zfields, "_id", 1);

  MAKE_STD_ZVAL(zcursor);

  PUSH_PARAM(criteria); PUSH_PARAM(zfields); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_MongoCollection_find(2, zcursor, &zcursor, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&zfields);

  chunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  MAKE_STD_ZVAL(next);
  zim_MongoCursor_getNext(0, next, &next, zcursor, return_value_used TSRMLS_CC);

  while (Z_TYPE_P(next) != IS_NULL) {
    zval **id;
    zval *temp;

    if (zend_hash_find(Z_ARRVAL_P(next), "_id", 4, (void**)&id) == FAILURE) {
      // uh oh
      continue;
    }

    MAKE_STD_ZVAL(temp);
    array_init(temp);
    zval_add_ref(id);
    add_assoc_zval(temp, "files_id", *id);

 
    PUSH_PARAM(temp); PUSH_PARAM((void*)1);
    PUSH_EO_PARAM();
    zim_MongoCollection_remove(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
    POP_EO_PARAM();
    POP_PARAM(); POP_PARAM();

    zval_ptr_dtor(&temp);
    zval_ptr_dtor(&next);
    MAKE_STD_ZVAL(next);
    zim_MongoCursor_getNext(0, next, &next, zcursor, return_value_used TSRMLS_CC);
  }
  zval_ptr_dtor(&next);
  zval_ptr_dtor(&zcursor);

  Z_TYPE(zjust_one) = IS_BOOL;
  zjust_one.value.lval = just_one;

  PUSH_PARAM(criteria); PUSH_PARAM(&zjust_one); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_MongoCollection_remove(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&criteria);
}

PHP_METHOD(MongoGridFS, storeUpload) {
  zval *filename, *h, *extra;
  zval **file, **temp;
  char *new_name = 0;
  int new_len = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|s", &filename, &new_name, &new_len) == FAILURE) {
    return;
  }
  convert_to_string(filename);

  h = PG(http_globals)[TRACK_VARS_FILES];
  if (zend_hash_find(Z_ARRVAL_P(h), Z_STRVAL_P(filename), Z_STRLEN_P(filename)+1, (void**)&file) == FAILURE) {
    zend_throw_exception_ex(mongo_ce_GridFSException, 0 TSRMLS_CC, "could not find uploaded file %s", Z_STRVAL_P(filename));
    return;
  }

  zend_hash_find(Z_ARRVAL_PP(file), "tmp_name", strlen("tmp_name")+1, (void**)&temp);
  convert_to_string(*temp);

  if (!new_name) {
    zval **n;
    zend_hash_find(Z_ARRVAL_PP(file), "name", strlen("name")+1, (void**)&n);
    new_name = Z_STRVAL_PP(n);
  }

  MAKE_STD_ZVAL(extra);
  array_init(extra);
  add_assoc_string(extra, "filename", new_name, 1);

  PUSH_PARAM(*temp); PUSH_PARAM(extra); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_MongoGridFS_storeFile(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&extra);
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
  ce.create_object = mongo_mongo_collection_new;
  mongo_ce_GridFS = zend_register_internal_class_ex(&ce, mongo_ce_Collection, "MongoCollection" TSRMLS_CC);

  zend_declare_property_null(mongo_ce_GridFS, "chunks", strlen("chunks"), ZEND_ACC_PUBLIC TSRMLS_CC);

  zend_declare_property_null(mongo_ce_GridFS, "filesName", strlen("filesName"), ZEND_ACC_PROTECTED TSRMLS_CC);
  zend_declare_property_null(mongo_ce_GridFS, "chunksName", strlen("chunksName"), ZEND_ACC_PROTECTED TSRMLS_CC);
}


PHP_METHOD(MongoGridFSFile, __construct) {
  zval *gridfs = 0, *file = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oa", &gridfs, mongo_ce_GridFS, &file) == FAILURE) {
    return;
  }

  zend_update_property(mongo_ce_GridFSFile, getThis(), "gridfs", strlen("gridfs"), gridfs TSRMLS_CC);
  zend_update_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), file TSRMLS_CC);
}

PHP_METHOD(MongoGridFSFile, getFilename) {
  zval *file = zend_read_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), NOISY TSRMLS_CC);
  zend_hash_find(Z_ARRVAL_P(file), "filename", strlen("filename")+1, (void**)&return_value_ptr);
  RETURN_STRING(Z_STRVAL_PP(return_value_ptr), 1);
}

PHP_METHOD(MongoGridFSFile, getSize) {
  zval *file = zend_read_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), NOISY TSRMLS_CC);
  zend_hash_find(Z_ARRVAL_P(file), "length", strlen("length")+1, (void**)&return_value_ptr);
  RETURN_LONG(Z_LVAL_PP(return_value_ptr));
}

PHP_METHOD(MongoGridFSFile, write) {
  char *filename = 0;
  int filename_len, total = 0;
  zval *gridfs, *file, *chunks, *n, *query, *cursor, *sort, *next;
  zval **id;
  FILE *fp;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &filename, &filename_len) == FAILURE) {
    return;
  }

  gridfs = zend_read_property(mongo_ce_GridFSFile, getThis(), "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);
  file = zend_read_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), NOISY TSRMLS_CC);

  // make sure that there's an index on chunks so we can sort by chunk num
  chunks = zend_read_property(mongo_ce_GridFS, gridfs, "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  MAKE_STD_ZVAL(n);
  array_init(n);
  add_assoc_long(n, "n", 1);  

  PUSH_PARAM(n); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCollection_ensureIndex(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&n);

  if (!filename) {
    zval **temp;
    zend_hash_find(Z_ARRVAL_P(file), "filename", strlen("filename")+1, (void**)&temp);

    filename = Z_STRVAL_PP(temp);
  }

  
  fp = fopen(filename, "wb");
  if (!fp) {
    zend_throw_exception_ex(mongo_ce_GridFSException, 0 TSRMLS_CC, "could not open destination file %s", filename);
    return;
  }

  zend_hash_find(Z_ARRVAL_P(file), "_id", strlen("_id")+1, (void**)&id);

  MAKE_STD_ZVAL(query);
  array_init(query);
  zval_add_ref(id);
  add_assoc_zval(query, "files_id", *id);

  MAKE_STD_ZVAL(cursor);

  PUSH_PARAM(query); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCollection_find(1, cursor, &cursor, chunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  MAKE_STD_ZVAL(sort);
  array_init(sort);
  add_assoc_long(sort, "n", 1);

  PUSH_PARAM(sort); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCursor_sort(1, cursor, &cursor, cursor, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  MAKE_STD_ZVAL(next);

  zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);

  while (Z_TYPE_P(next) != IS_NULL) {
    zval **zdata;
    int written;

    // check if data field exists.  if it doesn't, we've probably
    // got an error message from the db, so return that
    if (zend_hash_find(Z_ARRVAL_P(next), "data", 5, (void**)&zdata) == FAILURE) {
      zend_throw_exception(mongo_ce_GridFSException, "error reading chunk of file", 0 TSRMLS_CC);
      if(zend_hash_exists(Z_ARRVAL_P(next), "$err", 5)) {
        fclose(fp);
        return;
      }
      continue;
    }

    written = fwrite(Z_STRVAL_PP(zdata), 1, Z_STRLEN_PP(zdata), fp);
    if (written != Z_STRLEN_PP(zdata)) {
      zend_error(E_WARNING, "incorrect byte count.  expected: %d, got %d", Z_STRLEN_PP(zdata), written);
    }
    total += written;

    zval_ptr_dtor(&next);

    MAKE_STD_ZVAL(next);
    zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);
  }

  zval_ptr_dtor(&cursor);
  zval_ptr_dtor(&sort);
  zval_ptr_dtor(&query);
  zval_ptr_dtor(&next);

  fclose(fp);

  RETURN_LONG(total);
}

PHP_METHOD(MongoGridFSFile, getBytes) {
  zval *file, *gridfs, *chunks, *n, *query, *cursor, *sort, *next;
  zval **id, **size;
  char *str, *str_ptr;

  file = zend_read_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), NOISY TSRMLS_CC);
  zend_hash_find(Z_ARRVAL_P(file), "filename", strlen("filename")+1, (void**)&id);

  if (zend_hash_find(Z_ARRVAL_P(file), "length", strlen("length")+1, (void**)&size) == FAILURE) {
    zend_throw_exception(mongo_ce_GridFSException, "couldn't find file size", 0 TSRMLS_CC);
    return;
  }

  // make sure that there's an index on chunks so we can sort by chunk num
  gridfs = zend_read_property(mongo_ce_GridFSFile, getThis(), "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);
  chunks = zend_read_property(mongo_ce_GridFS, gridfs, "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  MAKE_STD_ZVAL(n);
  array_init(n);
  add_assoc_long(n, "n", 1);  

  PUSH_PARAM(n); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCollection_ensureIndex(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&n);

  // query for chunks
  MAKE_STD_ZVAL(query);
  array_init(query);
  zval_add_ref(id);
  add_assoc_zval(query, "files_id", *id);

  MAKE_STD_ZVAL(cursor);

  PUSH_PARAM(query); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCollection_find(1, cursor, &cursor, chunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  MAKE_STD_ZVAL(sort);
  array_init(sort);
  add_assoc_long(sort, "n", 1);

  PUSH_PARAM(sort); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  zim_MongoCursor_sort(1, cursor, &cursor, cursor, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  str = (char*)emalloc(Z_LVAL_PP(size));
  str_ptr = str;

  MAKE_STD_ZVAL(next);

  zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);
  while (Z_TYPE_P(next) != IS_NULL) {
    zval **zdata;

    // check if data field exists.  if it doesn't, we've probably
    // got an error message from the db, so return that
    if (zend_hash_find(Z_ARRVAL_P(next), "data", 5, (void**)&zdata) == FAILURE) {
      zend_throw_exception(mongo_ce_GridFSException, "error reading chunk of file", 0 TSRMLS_CC);
      if(zend_hash_exists(Z_ARRVAL_P(next), "$err", 5)) {
        return;
      }
      continue;
    }

    memcpy(str, Z_STRVAL_PP(zdata), Z_STRLEN_PP(zdata));
    str += Z_STRLEN_PP(zdata);

    zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);
  }

  zval_ptr_dtor(&next);

  RETURN_STRING(str_ptr, 0);
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


PHP_METHOD(MongoGridFSCursor, __construct) {
  zval *gridfs = 0, *connection = 0, *ns = 0, *query = 0, *fields = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ozzzz", &gridfs, mongo_ce_GridFS, &connection, &ns, &query, &fields) == FAILURE) {
    return;
  }

  zend_update_property(mongo_ce_GridFSCursor, getThis(), "gridfs", strlen("gridfs"), gridfs TSRMLS_CC);

  PUSH_PARAM(connection); PUSH_PARAM(ns); PUSH_PARAM(query); PUSH_PARAM(fields); PUSH_PARAM((void*)4);
  PUSH_EO_PARAM();
  zim_MongoCursor___construct(4, NULL, NULL, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();
}

PHP_METHOD(MongoGridFSCursor, getNext) {
  zim_MongoCursor_next(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  zim_MongoGridFSCursor_current(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
}

PHP_METHOD(MongoGridFSCursor, current) {
  zval temp;
  zval *gridfs;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (!cursor->current) {
    RETURN_NULL();
  }

  object_init_ex(return_value, mongo_ce_GridFSFile);

  gridfs = zend_read_property(mongo_ce_GridFSCursor, getThis(), "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);

  PUSH_PARAM(gridfs); PUSH_PARAM(cursor->current); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  zim_MongoGridFSFile___construct(2, &temp, NULL, return_value, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}

PHP_METHOD(MongoGridFSCursor, key) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (!cursor->current) {
    RETURN_NULL();
  }
  zend_hash_find(Z_ARRVAL_P(cursor->current), "filename", strlen("filename")+1, (void**)&return_value_ptr);
  if (!return_value_ptr) {
    RETURN_NULL();
  }
  convert_to_string(*return_value_ptr);
  RETURN_STRING(Z_STRVAL_PP(return_value_ptr), 1);
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

  zend_declare_property_null(mongo_ce_GridFSCursor, "gridfs", strlen("gridfs"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
