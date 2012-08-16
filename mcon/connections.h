#ifndef __MCON_CONNECTION_H__
#define __MCON_CONNECTION_H__

#include "types.h"
#include "str.h"

mongo_connection *mongo_connection_create(mongo_con_manager *manager, mongo_server_def *server_def, char **error_message);

int mongo_connection_get_reqid(mongo_connection *con);
int mongo_connection_ping(mongo_con_manager *manager, mongo_connection *con);
int mongo_connection_rs_status(mongo_con_manager *manager, mongo_connection *con, char **repl_set_name, int *nr_hosts, char ***found_hosts, char **error_message, mongo_server_def *server);
int mongo_connection_get_server_flags(mongo_con_manager *manager, mongo_connection *con, char **error_message);
void mongo_connection_destroy(mongo_con_manager *manager, mongo_connection *con);

#endif
