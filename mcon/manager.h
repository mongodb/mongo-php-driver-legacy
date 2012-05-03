#ifndef __MCON_MANAGER_H__
#define __MCON_MANAGER_H__

#include "types.h"

/* Manager */
mongo_con_manager *mongo_init(void);
void mongo_deinit(mongo_con_manager *manager);

/* Fetching connections */
static mongo_connection *mongo_get_connection_standalone(mongo_con_manager *manager, mongo_servers *servers);
mongo_connection *mongo_get_connection(mongo_con_manager *manager, mongo_servers *servers);

/* Connection management */
mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash);
mongo_connection *mongo_manager_connection_register(mongo_con_manager *manager, char *hash, mongo_connection *con);
#endif
