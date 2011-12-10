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

static size_t gridfs_read(php_stream *stream, char *buf, size_t count TSRMLS_DC);
static int gridfs_close(php_stream *stream, int close_handle TSRMLS_DC);

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

typedef struct _gridfs_stream_data {
   zval * file;
   size_t offset;

   /* file size */
   int size; 

   /* chunk size */
   int chunkSize;

   unsigned char * buffer;
   int buffer_size;
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

#define READ_PROPERTY(name, dest) \
    if (zend_hash_find(HASH_P(file), name, strlen(name)+1, (void**)&dest) == FAILURE) { \
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
    zval * file, **id, **size, **chunkSize;

    file = zend_read_property(mongo_ce_GridFSFile, file_object, "file", strlen("file"), NOISY TSRMLS_CC);
    READ_PROPERTY("_id", id);
    READ_PROPERTY("length", size);
    READ_PROPERTY("chunkSize", chunkSize);

    /* allocate memory and init the stream resource */
    self = emalloc(sizeof(*self));
    memset(self, 0, sizeof(*self));
    self->file = file_object;
    TO_INT(size, self->size);
    TO_INT(chunkSize, self->chunkSize);


    zval_add_ref(&file_object);

    stream = php_stream_alloc_rel(&gridfs_stream_ops, self, 0, "rb");
    return stream;
}

static size_t gridfs_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
{
    gridfs_stream_data * self = stream->abstract;


    printf("I'm reading\n");fflush(stdout);
}

static int gridfs_close(php_stream *stream, int close_handle TSRMLS_DC)
{
    gridfs_stream_data * self = stream->abstract;

    zval_ptr_dtor(&self->file);
    efree(self);

    return 0;
}
