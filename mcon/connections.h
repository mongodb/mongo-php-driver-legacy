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
#ifndef __MCON_CONNECTION_H__
#define __MCON_CONNECTION_H__

#include "types.h"
#include "str.h"
#ifndef WIN32
# include <sys/time.h>
#endif


char *mongo_authenticate_hash_user_password(char *username, char *password);
void mongo_connection_close(mongo_connection *con, int why);
void* mongo_connection_connect(mongo_con_manager *manager, mongo_server_def *server, mongo_server_options *options, char **error_message);
mongo_connection *mongo_connection_create(mongo_con_manager *manager, char *hash, mongo_server_def *server_def, mongo_server_options *options, char **error_message);

int mongo_connection_get_reqid(mongo_connection *con);
int mongo_connection_ping_check(mongo_con_manager *manager, int last_ping, struct timeval *start);
int mongo_connection_ping(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char **error_message);
int mongo_connection_ismaster(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char **repl_set_name, int *nr_hosts, char ***found_hosts, char **error_message, mongo_server_def *server);
int mongo_connection_get_server_flags(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char **error_message);
int mongo_connection_get_server_version(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char **error_message);
char *mongo_connection_getnonce(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char **error_message);
int mongo_connection_authenticate_mongodb_cr(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char *database, char *username, char *password, char *nonce, char **error_message);
int mongo_connection_authenticate_mongodb_x509(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char *database, char *username, char **error_message);
int mongo_connection_authenticate(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, char **error_message);
int mongo_connection_authenticate_cmd(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, char *database, char *username, mcon_str *packet, char **error_message);
int mongo_connection_authenticate_saslstart(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, char *mechanism, char *payload, unsigned int payload_len, char **out_payload, int *out_payload_len, int32_t *out_conversation_id, char **error_message);
int mongo_connection_authenticate_saslcontinue(mongo_con_manager *manager, mongo_connection *con, mongo_server_options *options, mongo_server_def *server_def, int32_t conversation_id, char *payload, int payload_len, char **out_payload, int *out_payload_len, unsigned char *done, char **error_message);
void mongo_connection_destroy(mongo_con_manager *manager, void *con, int why);
void mongo_connection_forget(mongo_con_manager *manager, mongo_connection *con);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
