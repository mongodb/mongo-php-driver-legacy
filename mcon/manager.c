#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "utils.h"
#include "manager.h"
#include "connections.h"

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
			mongo_connection_ping(con);
			mongo_manager_connection_register(manager, hash, con);
		}
	}
	free(hash);
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

/* - Helpers */
mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash)
{
	mongo_con_manager_item *ptr = manager->connections;

	while (ptr) {
		printf("found connection %s (looking for %s)\n", ptr->hash, hash);
		if (strcmp(ptr->hash, hash) == 0) {
			return ptr->connection;
		}
		ptr = ptr->next;
	}
	return NULL;
}

static mongo_con_manager_item *create_new_manager_item(void)
{
	mongo_con_manager_item *tmp = malloc(sizeof(mongo_con_manager_item));
	memset(tmp, 0, sizeof(mongo_con_manager_item));

	return tmp;
}

static void destroy_manager_item(mongo_con_manager_item *item)
{
	printf("freeing connection %s\n", item->hash);
	if (item->next) {
		destroy_manager_item(item->next);
	}
	mongo_connection_destroy(item->connection);
	free(item->hash);
	free(item);
}

mongo_connection *mongo_manager_connection_register(mongo_con_manager *manager, char *hash, mongo_connection *con)
{
	mongo_con_manager_item *ptr = manager->connections;
	mongo_con_manager_item *new;

	/* Setup new entry */
	new = create_new_manager_item();
	new->hash = strdup(hash);
	new->connection = con;
	new->next = NULL;

	if (!ptr) { /* No connections at all yet */
		manager->connections = new;
	} else {
		/* Existing connections, so find the last one */
		do {
			if (!ptr->next) {
				ptr->next = new;
				break;
			}
			ptr = ptr->next;
		} while (1);
	}
}

/* Init/deinit */
mongo_con_manager *mongo_init(void)
{
	mongo_con_manager *tmp;

	tmp = malloc(sizeof(mongo_con_manager));
	memset(tmp, 0, sizeof(mongo_con_manager));

	return tmp;
}

void mongo_deinit(mongo_con_manager *manager)
{
	if (manager->connections) {
		/* Does this recursively for all cons */
		destroy_manager_item(manager->connections);
	}

	free(manager);
}
