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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wire_version.h"

int php_mongo_api_supports_wire_version(int min_wire_version, int max_wire_version, char **error_message)
{
	char *errmsg = "This driver version requires WireVersion between minWireVersion: %d and maxWireVersion: %d. Got: minWireVersion=%d and maxWireVersion=%d";
	int errlen = strlen(errmsg) - 8 + 1 + (4 * 10); /* Subtract the %d, plus \0, plus 4 ints at maximum size.. */

	if (min_wire_version > PHP_MONGO_API_MAX_WIRE_VERSION) {
		*error_message = malloc(errlen);
		snprintf(*error_message, errlen, errmsg, PHP_MONGO_API_MIN_WIRE_VERSION, PHP_MONGO_API_MAX_WIRE_VERSION, min_wire_version, max_wire_version);
		return 0;
	}

	if (max_wire_version < PHP_MONGO_API_MIN_WIRE_VERSION) {
		*error_message = malloc(errlen);
		snprintf(*error_message, errlen, errmsg, PHP_MONGO_API_MIN_WIRE_VERSION, PHP_MONGO_API_MAX_WIRE_VERSION, min_wire_version, max_wire_version);
		return 0;
	}

	return 1;
}

int php_mongo_api_connection_supports_feature(mongo_connection *connection, int feature)
{
	if (connection->min_wire_version > feature) {
		return 0;
	}
	if (connection->max_wire_version < feature) {
		return 0;
	}

	return 1;
}

int php_mongo_api_connection_min_server_version(mongo_connection *connection, int major, int minor, int mini)
{
	if (connection->version.major > major) {
		return 1;
	}
	if (connection->version.major == major && connection->version.minor > minor) {
		return 1;
	}
	if (connection->version.major == major && connection->version.minor == minor && connection->version.mini >= mini) {
		return 1;
	}
	return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
