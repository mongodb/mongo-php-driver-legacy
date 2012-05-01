#include "types.h"
#include "utils.h"
#include "parse.h"
#include "manager.h"
#include "connections.h"

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

mongo_connection *mongo_connection_create(mongo_server_def *server_def)
{
}
