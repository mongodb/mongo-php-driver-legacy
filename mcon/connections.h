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
#ifndef __MCON_CONNECTION_H__
#define __MCON_CONNECTION_H__

#include "types.h"
#include "str.h"

mongo_connection *mongo_connection_create(mongo_con_manager *manager, mongo_server_def *server_def, mongo_server_opts *options, char **error_message);

int mongo_connection_get_reqid(mongo_connection *con);
int mongo_connection_ping(mongo_con_manager *manager, mongo_connection *con, char **error_message);
int mongo_connection_ismaster(mongo_con_manager *manager, mongo_connection *con, char **repl_set_name, int *nr_hosts, char ***found_hosts, char **error_message, mongo_server_def *server);
int mongo_connection_get_server_flags(mongo_con_manager *manager, mongo_connection *con, char **error_message);
char *mongo_connection_getnonce(mongo_con_manager *manager, mongo_connection *con, char **error_message);
int mongo_connection_authenticate(mongo_con_manager *manager, mongo_connection *con, char *database, char *username, char *password, char *nonce, char **error_message);
void mongo_connection_destroy(mongo_con_manager *manager, mongo_connection *con);

#endif
