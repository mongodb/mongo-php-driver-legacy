/**
 *  Copyright 2013-2014 MongoDB, Inc.
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

#ifndef __API_WIRE_VERSION_H__
#define __API_WIRE_VERSION_H__

#include "../mcon/types.h"

/* Known wire versions:
 *     MongoDB 2.4.0: N/A
 *     MongoDB 2.6.0: min=0, max=2
 */
#define PHP_MONGO_API_RELEASE_2_4_AND_BEFORE 0
#define PHP_MONGO_API_AGGREGATE_CURSOR       1
#define PHP_MONGO_API_WRITE_API              2
#define PHP_MONGO_API_RELEASE_2_8            3

/* Wire version this release of the driver supports */
#define PHP_MONGO_API_MIN_WIRE_VERSION 0
#define PHP_MONGO_API_MAX_WIRE_VERSION 3

int php_mongo_api_supports_wire_version(int min_wire_version, int max_wire_version, char **error_message);
int php_mongo_api_connection_supports_feature(mongo_connection *connection, int feature);

int php_mongo_api_connection_min_server_version(mongo_connection *connection, int major, int minor, int mini);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
