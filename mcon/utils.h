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
#ifndef __MCON_UTILS_H__
#define __MCON_UTILS_H__

#include "types.h"

char *mongo_server_create_hashed_password(char *username, char *password);
char *mongo_server_create_hash(mongo_server_def *server_def);
int mongo_server_split_hash(char *hash, char **host, int *port, char **repl_set_name, char **database, char **username, char **auth_hash, int *pid);
char *mongo_server_hash_to_server(char *hash);
int mongo_server_hash_to_pid(char *hash);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
