#include "collection.h"
#include "types.h"

/* Collecting the correct servers */

static mcon_collection *filter_connections(mongo_con_manager *manager, int types)
{
	mcon_collection *col;
	mongo_con_manager_item *ptr = manager->connections;

	col = mcon_init_collection(sizeof(mongo_connection*));

	while (ptr) {
		/* we need to check for username and pw on the connection too later */
		if (ptr->connection->connection_type & types) {
			printf("filter connections: adding type: %d, socket: %d, ping: %d\n",
				ptr->connection->connection_type,
				ptr->connection->socket,
				ptr->connection->ping_ms
			);
			mcon_collection_add(col, ptr->connection);
		}
		ptr = ptr->next;
	}

	return col;
}

mcon_collection *mongo_rp_collect_primary(mongo_con_manager *manager)
{
	return filter_connections(manager, MONGO_NODE_PRIMARY);
}

mcon_collection *mongo_rp_collect_secondary(mongo_con_manager *manager)
{
	return filter_connections(manager, MONGO_NODE_PRIMARY | MONGO_NODE_SECONDARY);
}

mcon_collection *mongo_rp_collect_secondary_only(mongo_con_manager *manager)
{
	return filter_connections(manager, MONGO_NODE_SECONDARY);
}

mcon_collection *mongo_rp_collect_any(mongo_con_manager *manager)
{
	return filter_connections(manager, MONGO_NODE_PRIMARY | MONGO_NODE_SECONDARY);
}
