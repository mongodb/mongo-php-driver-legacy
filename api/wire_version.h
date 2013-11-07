/**
 *  Copyright 2013 MongoDB, Inc.
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

/*
 * Known wire versions:
 * 0: All features before 2.5.0
 * 1: Aggregate can return cursors
 * 2: Write Commands API
 *
 * MongoDB 2.6.0 will return 0, 2
 */

/* Wire version this release of the driver supports */
#define PHP_MONGO_API_MIN_WIRE_VERSION 0
#define PHP_MONGO_API_MAX_WIRE_VERSION 2

int php_mongo_api_supports_wire_version(int min_wire_version, int max_wire_version, char **error_message);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
