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

#include "gridfs.h"
#include "collection.h"
#include "cursor.h"
#include "mongo.h"
#include "mongo_types.h"

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
  void *holder;
  zval *zdb;
  char *files = 0, *chunks = 0;
  int files_len = 0, chunks_len = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|ss", &zdb, mongo_ce_DB, &files, &files_len, &chunks, &chunks_len) == FAILURE) {
    return;
  }

  int release = 0;
  if (!files && !chunks) {
    files = "fs.files";
    chunks = "fs.chunks";
  }
  else if (!chunks) {
    release = 1;
    spprintf(&chunks, 0, "%s.chunks", files);
    spprintf(&files, 0, "%s.files", files);
  }

  zend_update_property_string(mongo_ce_GridFS, getThis(), "filesName", strlen("filesName"), files TSRMLS_CC);
  zend_update_property_string(mongo_ce_GridFS, getThis(), "chunksName", strlen("chunksName"), chunks TSRMLS_CC);

  zval *zfile;
  MAKE_STD_ZVAL(zfile);
  ZVAL_STRING(zfile, files, 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 4, zdb, zfile, 2, NULL);
  zim_MongoCollection___construct(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);

  zval *zchunks;
  MAKE_STD_ZVAL(zchunks);
  object_init_ex(zchunks, mongo_ce_Collection);

  zval *zchunk;
  MAKE_STD_ZVAL(zchunk);
  ZVAL_STRING(zchunk, chunks, 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 4, zdb, zchunk, 2, NULL);
  zim_MongoCollection___construct(2, return_value, return_value_ptr, zchunks, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
  
  zend_update_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), zchunks TSRMLS_CC);

  // ensure index on chunks.n
  zval *zidx;
  MAKE_STD_ZVAL(zidx);
  ZVAL_STRING(zidx, "n", 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, zidx, 1, NULL);
  zim_MongoCollection_ensureIndex(1, return_value, return_value_ptr, zchunks, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval_ptr_dtor(&zfile);
  zval_ptr_dtor(&zchunk);
  zval_ptr_dtor(&zchunks);
  zval_ptr_dtor(&zidx);

  if (release) {
    efree(files);
    efree(chunks);
  }

  zend_update_property(mongo_ce_GridFS, getThis(), "db", strlen("db"), zdb TSRMLS_CC);
}


PHP_METHOD(MongoGridFS, drop) {
  zval *zchunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);
  zval *zdb = zend_read_property(mongo_ce_GridFS, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, zchunks, 1, NULL);
  zim_MongoDB_dropCollection(1, return_value, return_value_ptr, zdb, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zim_MongoCollection_drop(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
}

PHP_METHOD(MongoGridFS, find) {
  zval *zquery = 0, *zfields = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|aa", &zquery, &zfields) == FAILURE) {
    return;
  }

  object_init_ex(return_value, mongo_ce_GridFSCursor);

  zval *zdb = zend_read_property(mongo_ce_GridFS, getThis(), "db", strlen("db"), NOISY TSRMLS_CC);
  zval *zlink = zend_read_property(mongo_ce_DB, zdb, "connection", strlen("connection"), NOISY TSRMLS_CC);
  zval *zns = zend_read_property(mongo_ce_GridFS, getThis(), "ns", strlen("ns"), NOISY TSRMLS_CC);

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

  zend_ptr_stack_n_push(&EG(argument_stack), 7, getThis(), zlink, zns, zquery, zfields, 5, NULL);

  zval temp;
  zim_MongoGridFSCursor___construct(5, &temp, NULL, return_value, return_value_used TSRMLS_CC);

  void *holder;
  zend_ptr_stack_n_pop(&EG(argument_stack), 7, &holder, &holder, &holder, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&zquery);
  zval_ptr_dtor(&zfields);
}

PHP_METHOD(MongoGridFS, storeFile) {
  char *filename;
  int filename_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
    return;
  }

  // try to open the file
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    char *errmsg;
    spprintf(&errmsg, 0, "could not open file %s", filename);
    zend_throw_exception(mongo_ce_GridFSException, errmsg, 0 TSRMLS_CC);
    efree(errmsg);
    return;
  }

  // get size
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  if (size >= 0xffffffff) {
    char *errmsg;
    spprintf(&errmsg, 0, "file %s is too large: %ld bytes", filename, size);
    zend_throw_exception(mongo_ce_GridFSException, errmsg, 0 TSRMLS_CC);
    efree(errmsg);
    return;
  }


  // create an id for the file
  zval temp;
  zval *zid;
  MAKE_STD_ZVAL(zid);
  object_init_ex(zid, mongo_ce_Id);
  zim_MongoId___construct(0, &temp, NULL, zid, return_value_used TSRMLS_CC);

  zval *zfile;
  MAKE_STD_ZVAL(zfile);
  array_init(zfile);
  
  long pos = 0;
  int chunk_num = 0, chunk_size;

  // reset file ptr
  fseek(fp, 0, SEEK_SET);

  // insert chunks
  zval *chunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);
  void *holder;
  while (pos < size) {
    chunk_size = size-pos >= MonGlo(chunk_size) ? MonGlo(chunk_size) : size-pos;
    char *buf = (char*)emalloc(chunk_size);
    if (fread(buf, 1, chunk_size, fp) < chunk_size) {
      char *errmsg;
      spprintf(&errmsg, 0, "error reading file %s", filename);
      zend_throw_exception(mongo_ce_GridFSException, errmsg, 0 TSRMLS_CC);
      efree(errmsg);
      return;
    }

    // create chunk
    zval *zchunk;
    MAKE_STD_ZVAL(zchunk);
    array_init(zchunk);

    add_assoc_zval(zchunk, "files_id", zid);
    add_assoc_long(zchunk, "n", chunk_num);
    add_assoc_stringl(zchunk, "data", buf, chunk_size, DUP);

    // insert chunk
    zend_ptr_stack_n_push(&EG(argument_stack), 3, zchunk, 1, NULL);
    zim_MongoCollection_insert(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
    zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);
    
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

  // get md5
  /*  zval *zmd5;
  ALLOC_INIT_ZVAL(zmd5);
  array_init(zmd5);

  add_assoc_zval(zmd5, "filemd5", id);
  add_assoc_string(zmd5, "root", gridfs->file_ns, DUP);

  char *cmd;
  spprintf(&cmd, 0, "%s.$cmd", gridfs->db);

  mongo_cursor *cursor = mongo_do_query(gridfs->link, cmd, 0, -1, zmd5, 0 TSRMLS_CC);

  zval_ptr_dtor(&zmd5);
  efree(cmd);
  if (!mongo_do_has_next(cursor TSRMLS_CC)) {
    zend_error(E_WARNING, "couldn't hash file %s\n", filename);
    RETURN_FALSE;
  }

  zval *hash = mongo_do_next(cursor TSRMLS_CC);
  add_assoc_zval(zfile, "md5", hash);
  */

  // insert file
  zend_ptr_stack_n_push(&EG(argument_stack), 3, zfile, 1, NULL);
  zim_MongoCollection_insert(1, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  //  zval_ptr_dtor(&hash);
  //  free_cursor(cursor);

  // cleanup
  zval_ptr_dtor(&zid);
  zval_ptr_dtor(&zfile);
}

PHP_METHOD(MongoGridFS, findOne) {
  zval *zquery = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &zquery) == FAILURE) {
    return;
  }

  if (!zquery) {
    MAKE_STD_ZVAL(zquery);
    array_init(zquery);
  }
  else if (Z_TYPE_P(zquery) != IS_ARRAY) {
    convert_to_string(zquery);

    zval *temp;
    MAKE_STD_ZVAL(temp);
    array_init(temp);
    add_assoc_string(temp, "filename", Z_STRVAL_P(zquery), 1);

    zquery = temp;
  }
  else {
    zval_add_ref(&zquery);
  }

  zval *file;
  MAKE_STD_ZVAL(file);

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 3, zquery, 1, NULL);
  zim_MongoCollection_findOne(1, file, &file, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  if (Z_TYPE_P(file) == IS_NULL) {
    RETVAL_ZVAL(file, 0, 1);
  }
  else {
    object_init_ex(return_value, mongo_ce_GridFSFile);

    zval temp;
    zend_ptr_stack_n_push(&EG(argument_stack), 4, getThis(), file, 2, NULL);
    zim_MongoGridFSFile___construct(2, &temp, NULL, return_value, return_value_used TSRMLS_CC);
    zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
  }

  zval_ptr_dtor(&file);
  zval_ptr_dtor(&zquery);
}


PHP_METHOD(MongoGridFS, remove) {
  zval *criteria = 0;
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

  zval *zfields;
  MAKE_STD_ZVAL(zfields);
  array_init(zfields);
  add_assoc_long(zfields, "_id", 1);

  zval *zcursor;
  MAKE_STD_ZVAL(zcursor);

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 4, criteria, zfields, 2, NULL);
  zim_MongoCollection_find(2, zcursor, &zcursor, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&zfields);

  zval *chunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  zval *next;
  MAKE_STD_ZVAL(next);
  zim_MongoCursor_getNext(0, next, &next, zcursor, return_value_used TSRMLS_CC);

  while (Z_TYPE_P(next) != IS_NULL) {
    zval **id;
    if (zend_hash_find(Z_ARRVAL_P(next), "_id", 4, (void**)&id) == FAILURE) {
      // uh oh
      continue;
    }

    zval *temp;
    MAKE_STD_ZVAL(temp);
    array_init(temp);
    zval_add_ref(id);
    add_assoc_zval(temp, "files_id", *id);


    zend_ptr_stack_n_push(&EG(argument_stack), 3, temp, 1, NULL);
    zim_MongoCollection_remove(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
    zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

    zval_ptr_dtor(&temp);
    zval_ptr_dtor(&next);
    MAKE_STD_ZVAL(next);
    zim_MongoCursor_getNext(0, next, &next, zcursor, return_value_used TSRMLS_CC);
  }
  zval_ptr_dtor(&next);
  zval_ptr_dtor(&zcursor);

  zval zjust_one;
  Z_TYPE(zjust_one) = IS_BOOL;
  zjust_one.value.lval = just_one;

  zend_ptr_stack_n_push(&EG(argument_stack), 4, criteria, &zjust_one, 2, NULL);
  zim_MongoCollection_remove(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);

  zval_ptr_dtor(&criteria);
}

PHP_METHOD(MongoGridFS, storeUpload) {

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
  int filename_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &filename, &filename_len) == FAILURE) {
    return;
  }

  zval *gridfs = zend_read_property(mongo_ce_GridFSFile, getThis(), "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);
  zval *file = zend_read_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), NOISY TSRMLS_CC);

  // make sure that there's an index on chunks so we can sort by chunk num
  zval *chunks = zend_read_property(mongo_ce_GridFS, gridfs, "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  zval *n;
  MAKE_STD_ZVAL(n);
  array_init(n);
  add_assoc_long(n, "n", 1);  

  void *holder;
  zend_ptr_stack_n_push(&EG(argument_stack), 3, n, 1, NULL);
  zim_MongoCollection_ensureIndex(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval_ptr_dtor(&n);

  if (!filename) {
    zval **temp;
    zend_hash_find(Z_ARRVAL_P(file), "filename", strlen("filename")+1, (void**)&temp);

    filename = Z_STRVAL_PP(temp);
  }

  
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    char *errmsg;
    spprintf(&errmsg, 0, "could not open destination file %s", filename);
    zend_throw_exception(mongo_ce_GridFSException, errmsg, 0 TSRMLS_CC);
    efree(errmsg);
    return;
  }

  zval **id;
  zend_hash_find(Z_ARRVAL_P(file), "_id", strlen("_id")+1, (void**)&id);

  zval *query;
  MAKE_STD_ZVAL(query);
  array_init(query);
  zval_add_ref(id);
  add_assoc_zval(query, "files_id", *id);

  zval *cursor;
  MAKE_STD_ZVAL(cursor);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, query, 1, NULL);
  zim_MongoCollection_find(1, cursor, &cursor, chunks, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval *sort;
  MAKE_STD_ZVAL(sort);
  array_init(sort);
  add_assoc_long(sort, "n", 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, sort, 1, NULL);
  zim_MongoCursor_sort(1, cursor, &cursor, cursor, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval *next;
  MAKE_STD_ZVAL(next);

  zim_MongoCursor_getNext(0, next, &next, cursor, return_value_used TSRMLS_CC);

  int total = 0;
  while (Z_TYPE_P(next) != IS_NULL) {
    zval **zdata;

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

    int written = fwrite(Z_STRVAL_PP(zdata), 1, Z_STRLEN_PP(zdata), fp);
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
  void *holder;

  zval *file = zend_read_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), NOISY TSRMLS_CC);
  zval **id;
  zend_hash_find(Z_ARRVAL_P(file), "filename", strlen("filename")+1, (void**)&id);

  zval **size;
  if (zend_hash_find(Z_ARRVAL_P(file), "length", strlen("length")+1, (void**)&size) == FAILURE) {
    zend_throw_exception(mongo_ce_GridFSException, "couldn't find file size", 0 TSRMLS_CC);
    return;
  }

  // make sure that there's an index on chunks so we can sort by chunk num
  zval *gridfs = zend_read_property(mongo_ce_GridFSFile, getThis(), "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);
  zval *chunks = zend_read_property(mongo_ce_GridFS, gridfs, "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  zval *n;
  MAKE_STD_ZVAL(n);
  array_init(n);
  add_assoc_long(n, "n", 1);  

  zend_ptr_stack_n_push(&EG(argument_stack), 3, n, 1, NULL);
  zim_MongoCollection_ensureIndex(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval_ptr_dtor(&n);

  // query for chunks
  zval *query;
  MAKE_STD_ZVAL(query);
  array_init(query);
  zval_add_ref(id);
  add_assoc_zval(query, "files_id", *id);

  zval *cursor;
  MAKE_STD_ZVAL(cursor);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, query, 1, NULL);
  zim_MongoCollection_find(1, cursor, &cursor, chunks, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  zval *sort;
  MAKE_STD_ZVAL(sort);
  array_init(sort);
  add_assoc_long(sort, "n", 1);

  zend_ptr_stack_n_push(&EG(argument_stack), 3, sort, 1, NULL);
  zim_MongoCursor_sort(1, cursor, &cursor, cursor, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 3, &holder, &holder, &holder);

  char *str = (char*)emalloc(Z_LVAL_PP(size));
  char *str_ptr = str;

  zval *next;
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
  void *holder;
  zval *gridfs = 0, *connection = 0, *ns = 0, *query = 0, *fields = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ozzzz", &gridfs, mongo_ce_GridFS, &connection, &ns, &query, &fields) == FAILURE) {
    return;
  }

  zend_update_property(mongo_ce_GridFSCursor, getThis(), "gridfs", strlen("gridfs"), gridfs TSRMLS_CC);

  zend_ptr_stack_n_push(&EG(argument_stack), 6, connection, ns, query, fields, 4, NULL);
  zim_MongoCursor___construct(4, NULL, NULL, getThis(), return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 6, &holder, &holder, &holder, &holder, &holder, &holder);
}

PHP_METHOD(MongoGridFSCursor, getNext) {
  zim_MongoCursor_next(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  zim_MongoGridFSCursor_current(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
}

PHP_METHOD(MongoGridFSCursor, current) {
  zval *current = zend_read_property(mongo_ce_GridFSCursor, getThis(), "current", strlen("current"), NOISY TSRMLS_CC);
  if (Z_TYPE_P(current) == IS_NULL) {
    RETURN_NULL();
  }

  object_init_ex(return_value, mongo_ce_GridFSFile);

  zval temp;
  void *holder;
  zval *gridfs = zend_read_property(mongo_ce_GridFSCursor, getThis(), "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);

  zend_ptr_stack_n_push(&EG(argument_stack), 4, gridfs, current, 2, NULL);
  zim_MongoGridFSFile___construct(2, &temp, NULL, return_value, return_value_used TSRMLS_CC);
  zend_ptr_stack_n_pop(&EG(argument_stack), 4, &holder, &holder, &holder, &holder);
}

PHP_METHOD(MongoGridFSCursor, key) {
  zval *current = zend_read_property(mongo_ce_GridFSCursor, getThis(), "current", strlen("current"), NOISY TSRMLS_CC);
  zend_hash_find(Z_ARRVAL_P(current), "filename", strlen("filename")+1, (void**)return_value_ptr);
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
