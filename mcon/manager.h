#ifndef __MCON_MANAGER_H__
#define __MCON_MANAGER_H__

#include "types.h"
#include "read_preference.h"

/* Manager */
mongo_con_manager *mongo_init(void);
void mongo_deinit(mongo_con_manager *manager);

/* Fetching connections */
mongo_connection *mongo_get_connection(mongo_con_manager *manager, mongo_servers *servers, mongo_read_preference *rp);

/* Connection management */
mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash);
void mongo_manager_connection_register(mongo_con_manager *manager, mongo_connection *con);
#endif
