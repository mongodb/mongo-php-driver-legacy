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

typedef struct _gridfs_stream_data {
   zval * this;
} gridfs_stream_data;

php_stream_ops gridfs_stream_ops = {
    NULL, /* write */
    gridfs_read, /* read */
    NULL, /* close */
    NULL, /* flush */
    "gridfs-wrapper",
    NULL, /* seek */
    NULL, /* cast */
    NULL, /* stat */
    NULL, /* set_option */
};


php_stream * gridfs_stream_init(zval * file_object) 
{
    gridfs_stream_data * self;
    php_stream * stream;

    self = emalloc(sizeof(*self));
    self->this = file_object;

    zval_add_ref(&file_object);

    stream = php_stream_alloc_rel(&gridfs_stream_ops, self, 0, "rb");
    stream->wrapperdata = file_object;

    return stream;
}

static size_t gridfs_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
{
    printf("I'm reading\n");
}
