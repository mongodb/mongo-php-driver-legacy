#include "manager.h"
#include "collection.h"
#include <stdlib.h>

int main(void)
{
	mongo_con_manager *manager;
	mongo_read_preference rp;
	mongo_connection *con;
	mcon_collection *collection;

	manager = mongo_init();

	/* Register connections */
	con = malloc(sizeof(mongo_connection));
	con->connection_type = MONGO_NODE_PRIMARY; con->socket = 1; con->ping_ms = 15;
	mongo_manager_connection_register(manager, "whisky:13000;X;10120", con);

	con = malloc(sizeof(mongo_connection));
	con->connection_type = MONGO_NODE_SECONDARY; con->socket = 2; con->ping_ms = 13;
	mongo_manager_connection_register(manager, "whisky:13001;X;10120", con);

	con = malloc(sizeof(mongo_connection));
	con->connection_type = MONGO_NODE_SECONDARY; con->socket = 3; con->ping_ms = 19;
	mongo_manager_connection_register(manager, "whisky:13002;X;10120", con);

	/* Configure RP */
	rp.type = MONGO_RP_PRIMARY_PREFERRED;
	collection = mongo_find_candidate_servers(manager, &rp);
	collection = mongo_select_server(collection, &rp);

	/* Cleaning up */
	mcon_collection_free(collection);	
	mongo_deinit(manager);

	return 0;
}
