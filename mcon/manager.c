#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "utils.h"
#include "manager.h"
#include "connections.h"

mongo_con_manager *mongo_init(void)
{
	mongo_con_manager *tmp;

	tmp = malloc(sizeof(mongo_con_manager));
	memset(tmp, 0, sizeof(mongo_con_manager));

	return tmp;
}

void mongo_deinit(mongo_con_manager *manager)
{
	free(manager);
}

/* Fetching connections */
static mongo_connection *mongo_get_connection_standalone(mongo_con_manager *manager, mongo_servers *servers)
{
	char *hash;
	mongo_connection *con;

	hash = mongo_server_create_hash(servers->server[0]);
	con = mongo_manager_connection_find_by_hash(manager, hash);
	if (!con) {
		con = mongo_connection_create(servers->server[0]);
		if (con) {
			mongo_manager_connection_register(manager, hash, con);
		}
	}
	return con;
}

mongo_connection *mongo_get_connection(mongo_con_manager *manager, mongo_servers *servers)
{
	/* Which connection we return depends on the type of connection we want */
	switch (servers->con_type) {
		case MONGO_CON_TYPE_STANDALONE:
			return mongo_get_connection_standalone(manager, servers);
/*
		case MONGO_CON_TYPE_REPLSET:
			return mongo_get_connection_replset(manager, servers);

		case MONGO_CON_TYPE_MULTIPLE:
			return mongo_get_connection_multiple(manager, servers);
*/
	}
}

/* Connection management */
mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash)
{
	return NULL;
}

mongo_connection *mongo_manager_connection_register(mongo_con_manager *manager, char *hash, mongo_connection *con)
{
}

