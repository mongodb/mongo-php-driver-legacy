//gridfs.c
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
// Author: César Rodas <crodas@php.net>
#include <php.h>
#ifdef WIN32
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#endif

#include <php_globals.h>
#include <ext/standard/file.h>
#include <ext/standard/flock_compat.h>
#ifdef HAVE_SYS_FILE_H
#   include <sys/file.h>
#endif

#include <zend_exceptions.h>

#include "php_mongo.h"
#include "gridfs.h"
#include "collection.h"
#include "cursor.h"
#include "mongo_types.h"
#include "db.h"

extern zend_class_entry *mongo_ce_DB,
    *mongo_ce_Collection,
    *mongo_ce_Cursor,
    *mongo_ce_Exception,
    *mongo_ce_GridFSException,
    *mongo_ce_Id,
    *mongo_ce_Date,
    *mongo_ce_BinData,
    *mongo_ce_GridFS,
    *mongo_ce_GridFSFile,
    *mongo_ce_GridFSCursor;

ZEND_EXTERN_MODULE_GLOBALS(mongo);

static size_t gridfs_read(php_stream *stream, char *buf, size_t count TSRMLS_DC);
static int gridfs_close(php_stream *stream, int close_handle TSRMLS_DC);

typedef struct _gridfs_stream_data {
    zval * fileObj; /* MongoGridFSFile Object */
    zval * chunkObj; /* Chunk collection object */
    zval * id; /* File ID  */
    zval * query; /* Query array */

    /* file current position */
    size_t offset;

    /* file size */
    int size; 

    /* chunk size */
    int chunkSize;

    /* mongo current chunk is kept in memory */
    unsigned char * buffer;

    /* chunk size */
    int buffer_size;

    /* where we are in the chunk? */
    size_t buffer_offset;
} gridfs_stream_data;

php_stream_ops gridfs_stream_ops = {
    NULL, /* write */
    gridfs_read, /* read */
    gridfs_close, /* close */
    NULL, /* flush */
    "gridfs-wrapper",
    NULL, /* seek */
    NULL, /* cast */
    NULL, /* stat */
    NULL, /* set_option */
};


#define READ_PROPERTY(dest, name, toVariable) \
    if (zend_hash_find(HASH_P(dest), name, strlen(name)+1, (void**)&toVariable) == FAILURE) { \
        zend_throw_exception(mongo_ce_GridFSException, "couldn't find " name, 0 TSRMLS_CC); \
        return; \
    } \

#define TO_INT(size, len) { \
  if (Z_TYPE_PP(size) == IS_DOUBLE) { \
    len = (int)Z_DVAL_PP(size); \
  } else {  \
    len = Z_LVAL_PP(size); \
  } \
} 

php_stream * gridfs_stream_init(zval * file_object) 
{
    gridfs_stream_data * self;
    php_stream * stream;
    zval * file, **id, **size, **chunkSize, *gridfs;

    file = zend_read_property(mongo_ce_GridFSFile, file_object, "file", strlen("file"), NOISY TSRMLS_CC);
    READ_PROPERTY(file, "_id", id);
    READ_PROPERTY(file, "length", size);
    READ_PROPERTY(file, "chunkSize", chunkSize);

    gridfs = zend_read_property(mongo_ce_GridFSFile, file_object, "gridfs", strlen("gridfs"), NOISY TSRMLS_CC);

    /* allocate memory and init the stream resource */
    self = emalloc(sizeof(*self));
    memset(self, 0, sizeof(*self));
    TO_INT(size, self->size);
    TO_INT(chunkSize, self->chunkSize);

    self->fileObj  = file_object;
    self->chunkObj = zend_read_property(mongo_ce_GridFS, gridfs, "chunks", strlen("chunks"), NOISY TSRMLS_CC);
    self->buffer   = emalloc(self->chunkSize +1);
    self->id = *id;

    zval_add_ref(&self->fileObj);
    zval_add_ref(&self->chunkObj);
    zval_add_ref(&self->id);


    /* create base query object */
    MAKE_STD_ZVAL(self->query);
    array_init(self->query);
    add_assoc_zval(self->query, "files_id", self->id);
    zval_add_ref(&self->id);

    stream = php_stream_alloc_rel(&gridfs_stream_ops, self, 0, "rb");
    return stream;
}

static int copy_bytes(void *to, char *from, int len) {
  char *winIsDumb = *(char**)to;
  memcpy(winIsDumb, from, len);
  winIsDumb += len;
  *((char**)to) = (void*)winIsDumb;

  return len;
}

#define ASSERT_SIZE(size) \
    if (size > self->chunkSize) { \
        char * err; \
        spprintf(&err, 0, "chunk %d has wrong size (%d) when the max is %d", chunk_id, size, self->chunkSize); \
        zend_throw_exception(mongo_ce_GridFSException, err, 1 TSRMLS_CC); \
        return FAILURE; \
    } \

static int gridfs_read_chunk(gridfs_stream_data *self, int chunk_id TSRMLS_DC)
{
    zval * chunk = 0, *zfields, **data, *bin;

    if (chunk_id == -1) {
        chunk_id = (int)(self->offset / self->chunkSize);
    }

    MAKE_STD_ZVAL(zfields);
    array_init(zfields);

    add_assoc_long(self->query, "n", chunk_id);

    MAKE_STD_ZVAL(chunk);
    MONGO_METHOD2(MongoCollection, findOne, chunk, self->chunkObj, self->query, zfields);

    if (Z_TYPE_P(chunk) == IS_NULL) {
        char * err;
        spprintf(&err, 0, "couldn't fink file chunk %d", chunk_id);
        zend_throw_exception(mongo_ce_GridFSException, err, 1 TSRMLS_CC);
        return FAILURE;
    }

    READ_PROPERTY(chunk, "data", data);

    if (Z_TYPE_PP(data) == IS_STRING) {
        // old driver, not yet implemented
        ASSERT_SIZE(Z_STRLEN_PP(data))
        memcpy(self->buffer, Z_STRVAL_PP(data), Z_STRLEN_PP(data));
        self->buffer_size = Z_STRLEN_PP(data);
    } else if (Z_TYPE_PP(data) == IS_OBJECT &&
             Z_OBJCE_PP(data) == mongo_ce_BinData) {

        bin = zend_read_property(mongo_ce_BinData, *data, "bin", strlen("bin"), NOISY TSRMLS_CC);
        ASSERT_SIZE(Z_STRLEN_P(bin))
        memcpy(self->buffer, Z_STRVAL_P(bin), Z_STRLEN_P(bin));
        self->buffer_size = Z_STRLEN_P(bin);
    } else {
        zend_throw_exception(mongo_ce_GridFSException, "chunk has wrong format", 0 TSRMLS_CC);
        return FAILURE;
    }

    zval_ptr_dtor(&chunk);

    return SUCCESS;
}

static size_t gridfs_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
{
    gridfs_stream_data * self = stream->abstract;
    int size;

    if (self->buffer_size == 0) {
        // read the needed buffer
        gridfs_read_chunk(self, -1 TSRMLS_CC);
    }

    size = self->buffer_size;

    memcpy(buf, self->buffer, size);

    return size;
}

static int gridfs_close(php_stream *stream, int close_handle TSRMLS_DC)
{
    gridfs_stream_data * self = stream->abstract;

    zval_ptr_dtor(&self->fileObj);
    zval_ptr_dtor(&self->chunkObj);
    zval_ptr_dtor(&self->buffer);
    zval_ptr_dtor(&self->query);
    zval_ptr_dtor(&self->id);
    efree(self);

    return 0;
}
