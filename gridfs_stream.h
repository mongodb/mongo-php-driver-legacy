/**
 *  Copyright 2009-2012 10gen, Inc.
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
#ifndef MONGO_GRIDFS_STREAM_H
#define MONGO_GRIDFS_STREAM_H

PHPAPI php_stream * gridfs_stream_init(zval * file_object TSRMLS_DC);

#endif /* MONGO_GRIDFS_STREAM_H */
