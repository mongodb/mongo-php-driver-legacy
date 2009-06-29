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
#include "php_mongo.h"
#include "mongo_types.h"
#include "db.h"

extern zend_class_entry *mongo_ce_DB,
  *mongo_ce_Collection,
  *mongo_ce_Cursor,
  *mongo_ce_Exception,
  *mongo_ce_GridFSException,
  *mongo_ce_Id,
  *mongo_ce_Date,
  *mongo_ce_BinData;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

zend_class_entry *mongo_ce_GridFS = NULL,
  *mongo_ce_GridFSFile = NULL,
  *mongo_ce_GridFSCursor = NULL;


typedef int (*apply_copy_func_t)(void *to, char *from, int len);

static int copy_bytes(void *to, char *from, int len);
static int copy_file(void *to, char *from, int len);

static int apply_to_cursor(zval *cursor, apply_copy_func_t apply_copy_func, void *to TSRMLS_DC);
static int setup_file(FILE *fpp, char *filename TSRMLS_DC);
static int get_chunk_size(zval *array TSRMLS_DC);


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
  MONGO_METHOD(MongoCollection, __construct)(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); 

  // create chunks collection
  MAKE_STD_ZVAL(zchunks);
  object_init_ex(zchunks, mongo_ce_Collection);

  PUSH_PARAM(zdb); PUSH_PARAM(chunks); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoCollection, __construct)(2, return_value, return_value_ptr, zchunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); 
  
  // add chunks collection as a property
  zend_update_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), zchunks TSRMLS_CC);
  
  // ensure index on chunks.n
  MAKE_STD_ZVAL(zidx);
  array_init(zidx);
  add_assoc_long(zidx, "n", 1);

  PUSH_PARAM(zidx); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoCollection, ensureIndex)(1, return_value, return_value_ptr, zchunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  zend_update_property(mongo_ce_GridFS, getThis(), "filesName", strlen("filesName"), files TSRMLS_CC);
  zend_update_property(mongo_ce_GridFS, getThis(), "chunksName", strlen("chunksName"), chunks TSRMLS_CC);

  // cleanup
  zval_ptr_dtor(&zchunks);
  zval_ptr_dtor(&zidx);

  zval_ptr_dtor(&files);
  zval_ptr_dtor(&chunks);
}


PHP_METHOD(MongoGridFS, drop) {
  zval *temp;
  zval *zchunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  MAKE_STD_ZVAL(temp);
  MONGO_METHOD(MongoCollection, drop)(0, temp, NULL, zchunks, return_value_used TSRMLS_CC);
  zval_ptr_dtor(&temp);
  
  MONGO_METHOD(MongoCollection, drop)(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
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
  MONGO_CHECK_INITIALIZED(c->ns, MongoGridFS);

  PUSH_PARAM(getThis()); PUSH_PARAM(c->db->link); PUSH_PARAM(c->ns); PUSH_PARAM(zquery); PUSH_PARAM(zfields); PUSH_PARAM((void*)5);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoGridFSCursor, __construct)(5, &temp, NULL, return_value, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&zquery);
  zval_ptr_dtor(&zfields);
}


static int get_chunk_size(zval *array TSRMLS_DC) {
  zval **zchunk_size = 0;

  if (zend_hash_find(Z_ARRVAL_P(array), "chunkSize", strlen("chunkSize")+1, (void**)&zchunk_size) == FAILURE) {
    add_assoc_long(array, "chunkSize", MonGlo(chunk_size));
    return MonGlo(chunk_size);
  }

  convert_to_long(*zchunk_size);
  return Z_LVAL_PP(zchunk_size) > 0 ? 
    Z_LVAL_PP(zchunk_size) :
    MonGlo(chunk_size);
}


static int setup_file(FILE *fp, char *filename TSRMLS_DC) {
  int size = 0;

  // try to open the file
  if (!fp) {
    zend_throw_exception_ex(mongo_ce_GridFSException, 0 TSRMLS_CC, "could not open file %s", filename);
    return FAILURE;
  }

  // get size
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  if (size >= 0xffffffff) {
    zend_throw_exception_ex(mongo_ce_GridFSException, 0 TSRMLS_CC, "file %s is too large: %ld bytes", filename, size);
    fclose(fp);
    return FAILURE;
  }

  // reset file ptr
  fseek(fp, 0, SEEK_SET);

  return size;
}

PHP_METHOD(MongoGridFS, storeBytes) {
  char *bytes = 0;
  int bytes_len = 0, chunk_num = 0, chunk_size = 0, global_chunk_size = 0, pos = 0;
  zend_bool created_date = 0, created_id = 0;

  zval temp;
  zval *extra = 0, *zid = 0, *zfile = 0, *chunks = 0;
  zval **zzid = 0;

  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoGridFS);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &bytes, &bytes_len, &extra) == FAILURE) {
    return;
  }

  // file array object
  MAKE_STD_ZVAL(zfile);
  array_init(zfile);

  // add user-defined fields
  if (extra) {
    zval temp;
    zend_hash_merge(Z_ARRVAL_P(zfile), Z_ARRVAL_P(extra), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);
  }

  // check if we need to add any fields

  // _id
  if (zend_hash_find(Z_ARRVAL_P(zfile), "_id", strlen("_id")+1, (void**)&zzid) == FAILURE) {
    // create an id for the file
    MAKE_STD_ZVAL(zid);
    object_init_ex(zid, mongo_ce_Id);
    MONGO_METHOD(MongoId, __construct)(0, &temp, NULL, zid, return_value_used TSRMLS_CC);

    add_assoc_zval(zfile, "_id", zid);
    created_id = 1;
  }
  else {
    zid = *zzid;
  }
  zval_add_ref(&zid);

  // size
  if (!zend_hash_exists(Z_ARRVAL_P(zfile), "length", strlen("length")+1)) {
    add_assoc_long(zfile, "length", bytes_len);
  }

  // chunkSize
  global_chunk_size = get_chunk_size(zfile TSRMLS_CC);

  // insert chunks
  chunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);
  while (pos < bytes_len) {
    zval *zchunk, *zbin;

    chunk_size = bytes_len-pos >= global_chunk_size ? global_chunk_size : bytes_len-pos;

    // create chunk
    MAKE_STD_ZVAL(zchunk);
    array_init(zchunk);

    add_assoc_zval(zchunk, "files_id", zid);
    add_assoc_long(zchunk, "n", chunk_num);

    // create MongoBinData object
    MAKE_STD_ZVAL(zbin);
    object_init_ex(zbin, mongo_ce_BinData);
    add_property_stringl(zbin, "bin", bytes+pos, chunk_size, DUP);
    add_property_long(zbin, "type", 2);

    add_assoc_zval(zchunk, "data", zbin);

    // insert chunk

    PUSH_PARAM(zchunk); PUSH_PARAM((void*)1);
    PUSH_EO_PARAM();
    MONGO_METHOD(MongoCollection, insert)(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
    POP_EO_PARAM();
    POP_PARAM(); POP_PARAM(); 
    
    // increment counters
    pos += chunk_size;
    chunk_num++; 

    zval_add_ref(&zid); // zid->refcount = 2
    zval_ptr_dtor(&zchunk); // zid->refcount = 1
  }

  // insert file
  PUSH_PARAM(zfile); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoCollection, insert)(1, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); 

  // cleanup
  if (created_id) {
    zend_objects_store_del_ref(zid TSRMLS_CC);
    zval_ptr_dtor(&zid);
  }
  zval_ptr_dtor(&zfile);
}


PHP_METHOD(MongoGridFS, storeFile) {
  char *filename = 0;
  int filename_len = 0, chunk_num = 0, chunk_size = 0, global_chunk_size = 0, size = 0, pos = 0;
  FILE *fp = 0;
  zend_bool created_date = 0, created_id = 0;

  zval temp;
  zval *extra = 0, *zid = 0, *zfile = 0, *chunks = 0, *upload_date = 0;
  zval **zzid = 0, **md5 = 0;

  mongo_collection *c = (mongo_collection*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(c->ns, MongoGridFS);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &filename, &filename_len, &extra) == FAILURE) {
    return;
  }

  fp = fopen(filename, "rb");
  // no point in continuing if we can't open the file
  if ((size = setup_file(fp, filename TSRMLS_CC)) == FAILURE) {
    return;
  }

  // file array object
  MAKE_STD_ZVAL(zfile);
  array_init(zfile);

  // add user-defined fields
  if (extra) {
    zval temp;
    zend_hash_merge(Z_ARRVAL_P(zfile), Z_ARRVAL_P(extra), (void (*)(void*))zval_add_ref, &temp, sizeof(zval*), 1);
  }

  // check if we need to add any fields

  // _id
  if (zend_hash_find(Z_ARRVAL_P(zfile), "_id", strlen("_id")+1, (void**)&zzid) == FAILURE) {
    // create an id for the file
    MAKE_STD_ZVAL(zid);
    object_init_ex(zid, mongo_ce_Id);
    MONGO_METHOD(MongoId, __construct)(0, &temp, NULL, zid, return_value_used TSRMLS_CC);

    add_assoc_zval(zfile, "_id", zid);
    created_id = 1;
  }
  else {
    zid = *zzid;
  }
  zval_add_ref(&zid);

  // filename
  if (!zend_hash_exists(Z_ARRVAL_P(zfile), "filename", strlen("filename")+1)) {
    add_assoc_stringl(zfile, "filename", filename, filename_len, DUP);
  }

  // size
  if (!zend_hash_exists(Z_ARRVAL_P(zfile), "length", strlen("length")+1)) {
    add_assoc_long(zfile, "length", size);
  }

  // chunkSize
  global_chunk_size = get_chunk_size(zfile TSRMLS_CC);

  // uploadDate
  if (!zend_hash_exists(Z_ARRVAL_P(zfile), "uploadDate", strlen("uploadDate")+1)) {
    // create an id for the file
    MAKE_STD_ZVAL(upload_date);
    object_init_ex(upload_date, mongo_ce_Date);
    MONGO_METHOD(MongoDate, __construct)(0, &temp, NULL, upload_date, return_value_used TSRMLS_CC);

    add_assoc_zval(zfile, "uploadDate", upload_date);
    created_date = 1;
  }

  // insert chunks
  chunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);
  while (pos < size) {
    char *buf;
    zval *zchunk, *zbin;

    chunk_size = size-pos >= global_chunk_size ? global_chunk_size : size-pos;
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

    // create MongoBinData object
    MAKE_STD_ZVAL(zbin);
    object_init_ex(zbin, mongo_ce_BinData);
    add_property_stringl(zbin, "bin", buf, chunk_size, DUP);
    add_property_long(zbin, "type", 2);

    add_assoc_zval(zchunk, "data", zbin);

    // insert chunk

    PUSH_PARAM(zchunk); PUSH_PARAM((void*)1);
    PUSH_EO_PARAM();
    MONGO_METHOD(MongoCollection, insert)(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
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


  // add chunks md5 hash
  if (!zend_hash_exists(Z_ARRVAL_P(zfile), "md5", strlen("md5")+1)) {
    zval *md5_cmd = 0, *response = 0;

    MAKE_STD_ZVAL(md5_cmd);
    array_init(md5_cmd);

    add_assoc_zval(md5_cmd, "filemd5", zid);
    zval_add_ref(&zid);
    add_assoc_zval(md5_cmd, "root", c->ns);
    zval_add_ref(&c->ns);

    MAKE_STD_ZVAL(response);
    PUSH_PARAM(md5_cmd); PUSH_PARAM((void*)1);
    PUSH_EO_PARAM();
    MONGO_METHOD(MongoDB, command)(1, response, NULL, c->parent, return_value_used TSRMLS_CC); 
    POP_EO_PARAM();
    POP_PARAM(); POP_PARAM();

    if (zend_hash_find(Z_ARRVAL_P(response), "md5", strlen("md5")+1, (void**)&md5) == SUCCESS) {
      add_assoc_zval(zfile, "md5", *md5);
      zval_add_ref(md5);
    }

    zval_ptr_dtor(&response);
    zval_ptr_dtor(&md5_cmd);
  }

  // insert file
  PUSH_PARAM(zfile); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoCollection, insert)(1, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); 

  // cleanup
  if (created_id) {
    zend_objects_store_del_ref(zid TSRMLS_CC);
  }
  if (created_date) {
    zend_objects_store_del_ref(upload_date TSRMLS_CC);
  }
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
  MONGO_METHOD(MongoCollection, findOne)(1, file, &file, getThis(), return_value_used TSRMLS_CC);
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
    MONGO_METHOD(MongoGridFSFile, __construct)(2, &temp, NULL, return_value, return_value_used TSRMLS_CC);
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
  MONGO_METHOD(MongoCollection, find)(2, zcursor, &zcursor, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&zfields);

  chunks = zend_read_property(mongo_ce_GridFS, getThis(), "chunks", strlen("chunks"), NOISY TSRMLS_CC);

  MAKE_STD_ZVAL(next);
  MONGO_METHOD(MongoCursor, getNext)(0, next, &next, zcursor, return_value_used TSRMLS_CC);

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
    MONGO_METHOD(MongoCollection, remove)(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
    POP_EO_PARAM();
    POP_PARAM(); POP_PARAM();

    zval_ptr_dtor(&temp);
    zval_ptr_dtor(&next);
    MAKE_STD_ZVAL(next);
    MONGO_METHOD(MongoCursor, getNext)(0, next, &next, zcursor, return_value_used TSRMLS_CC);
  }
  zval_ptr_dtor(&next);
  zval_ptr_dtor(&zcursor);

  Z_TYPE(zjust_one) = IS_BOOL;
  zjust_one.value.lval = just_one;

  PUSH_PARAM(criteria); PUSH_PARAM(&zjust_one); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoCollection, remove)(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
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
  MONGO_METHOD(MongoGridFS, storeFile)(2, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();

  zval_ptr_dtor(&extra);
}


static function_entry MongoGridFS_methods[] = {
  PHP_ME(MongoGridFS, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, drop, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, find, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, storeFile, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(MongoGridFS, storeBytes, NULL, ZEND_ACC_PUBLIC)
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
  zval *gridfs, *file, *chunks, *n, *query, *cursor, *sort;
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
  MONGO_METHOD(MongoCollection, ensureIndex)(1, return_value, return_value_ptr, chunks, return_value_used TSRMLS_CC);
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
  MONGO_METHOD(MongoCollection, find)(1, cursor, &cursor, chunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  MAKE_STD_ZVAL(sort);
  array_init(sort);
  add_assoc_long(sort, "n", 1);

  PUSH_PARAM(sort); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoCursor, sort)(1, cursor, &cursor, cursor, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  if ((total = apply_to_cursor(cursor, copy_file, fp TSRMLS_CC)) == FAILURE) {
    zend_throw_exception(mongo_ce_GridFSException, "error reading chunk of file", 0 TSRMLS_CC);
  }

  fclose(fp);

  zval_ptr_dtor(&cursor);
  zval_ptr_dtor(&sort); 
  zval_ptr_dtor(&query);

  RETURN_LONG(total);
}

PHP_METHOD(MongoGridFSFile, getBytes) {
  zval temp;
  zval *file, *gridfs, *chunks, *n, *query, *cursor, *sort;
  zval **id, **size;
  char *str, *str_ptr;
  int len;

  file = zend_read_property(mongo_ce_GridFSFile, getThis(), "file", strlen("file"), NOISY TSRMLS_CC);
  zend_hash_find(Z_ARRVAL_P(file), "_id", strlen("_id")+1, (void**)&id);

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
  MONGO_METHOD(MongoCollection, ensureIndex)(1, &temp, NULL, chunks, return_value_used TSRMLS_CC);
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
  MONGO_METHOD(MongoCollection, find)(1, cursor, &cursor, chunks, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  MAKE_STD_ZVAL(sort);
  array_init(sort);
  add_assoc_long(sort, "n", 1);

  PUSH_PARAM(sort); PUSH_PARAM((void*)1);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoCursor, sort)(1, &temp, NULL, cursor, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM();

  if (Z_TYPE_PP(size) == IS_DOUBLE) {
    len = (int)Z_DVAL_PP(size);
  }
  else { // if Z_TYPE_PP(size) == IS_LONG
    len = Z_LVAL_PP(size);
  }
  str = (char*)emalloc(len + 1);

  zval_ptr_dtor(&query);
  zval_ptr_dtor(&sort);
  str_ptr = str;

  if (apply_to_cursor(cursor, copy_bytes, str TSRMLS_CC) == FAILURE) {
    zend_throw_exception(mongo_ce_GridFSException, "error reading chunk of file", 0 TSRMLS_CC);
  }

  zval_ptr_dtor(&cursor);

  str[len] = '\0';

  RETURN_STRINGL(str_ptr, Z_LVAL_PP(size), 0);
}

static int copy_bytes(void *to, char *from, int len) {
  char *winIsDumb = (char*)to;
  memcpy(winIsDumb, from, len);
  winIsDumb += len;
  to = (void*)winIsDumb;

  return len;
}

static int copy_file(void *to, char *from, int len) {
  int written = fwrite(from, 1, len, (FILE*)to);

  if (written != len) {
    zend_error(E_WARNING, "incorrect byte count.  expected: %d, got %d", len, written);
  }

  return written;
}

static int apply_to_cursor(zval *cursor, apply_copy_func_t apply_copy_func, void *to TSRMLS_DC) {
  int total = 0;
  zval *next;

  MAKE_STD_ZVAL(next);                                                                     
  MONGO_METHOD(MongoCursor, getNext)(0, next, NULL, cursor, 0 TSRMLS_CC);

  while (Z_TYPE_P(next) != IS_NULL) {
    zval **zdata;

    // check if data field exists.  if it doesn't, we've probably
    // got an error message from the db, so return that
    if (zend_hash_find(Z_ARRVAL_P(next), "data", 5, (void**)&zdata) == FAILURE) {
      if(zend_hash_exists(Z_ARRVAL_P(next), "$err", 5)) {
        return FAILURE;
      }
      continue;
    }

    /* This copies the next chunk -> *to  
     * Due to a talent I have for not reading directions, older versions of the driver
     * store files as raw bytes, not MongoBinData.  So, we'll check for and handle 
     * both cases.
     */
    // raw bytes
    if (Z_TYPE_PP(zdata) == IS_STRING) {
      total += apply_copy_func(to, Z_STRVAL_PP(zdata), Z_STRLEN_PP(zdata));
    }
    // MongoBinData
    else if (Z_TYPE_PP(zdata) == IS_OBJECT &&
             Z_OBJCE_PP(zdata) == mongo_ce_BinData) {
      zval *bin = zend_read_property(mongo_ce_BinData, *zdata, "bin", strlen("bin"), NOISY TSRMLS_CC);
      total += apply_copy_func(to, Z_STRVAL_P(bin), Z_STRLEN_P(bin));
    }
    // if it's not a string or a MongoBinData, give up
    else {
      return FAILURE;
    }

    // get ready for the next iteration
    zval_ptr_dtor(&next);
    MAKE_STD_ZVAL(next);
    MONGO_METHOD(MongoCursor, getNext)(0, next, NULL, cursor, 0 TSRMLS_CC);
  }
  zval_ptr_dtor(&next);

  // return the number of bytes copied
  return total;
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
  MONGO_METHOD(MongoCursor, __construct)(4, NULL, NULL, getThis(), return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();
}

PHP_METHOD(MongoGridFSCursor, getNext) {
  MONGO_METHOD(MongoCursor, next)(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
  MONGO_METHOD(MongoGridFSCursor, current)(0, return_value, return_value_ptr, getThis(), return_value_used TSRMLS_CC);
}

PHP_METHOD(MongoGridFSCursor, current) {
  zval temp;
  zval *gridfs;
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoGridFSCursor);

  if (!cursor->current) {
    RETURN_NULL();
  }

  object_init_ex(return_value, mongo_ce_GridFSFile);

  gridfs = zend_read_property(mongo_ce_GridFSCursor, getThis(), "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);

  PUSH_PARAM(gridfs); PUSH_PARAM(cursor->current); PUSH_PARAM((void*)2);
  PUSH_EO_PARAM();
  MONGO_METHOD(MongoGridFSFile, __construct)(2, &temp, NULL, return_value, return_value_used TSRMLS_CC);
  POP_EO_PARAM();
  POP_PARAM(); POP_PARAM(); POP_PARAM();
}

PHP_METHOD(MongoGridFSCursor, key) {
  mongo_cursor *cursor = (mongo_cursor*)zend_object_store_get_object(getThis() TSRMLS_CC);
  MONGO_CHECK_INITIALIZED(cursor->link, MongoGridFSCursor);

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
