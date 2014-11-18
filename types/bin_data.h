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
#ifndef __TYPES_BIN_DATA_H__
#define __TYPES_BIN_DATA_H__

#define PHP_MONGO_BIN_GENERIC      0x00
#define PHP_MONGO_BIN_FUNC         0x01
#define PHP_MONGO_BIN_BYTE_ARRAY   0x02
#define PHP_MONGO_BIN_UUID         0x03
#define PHP_MONGO_BIN_UUID_RFC4122 0x04
#define PHP_MONGO_BIN_MD5          0x05
#define PHP_MONGO_BIN_CUSTOM       0x80

#define PHP_MONGO_BIN_UUID_RFC4122_SIZE 16

PHP_METHOD(MongoBinData, __construct);
PHP_METHOD(MongoBinData, __toString);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
