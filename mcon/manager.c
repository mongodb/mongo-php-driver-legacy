#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "utils.h"
#include "manager.h"
#include "connections.h"

/* Helpers */
static mongo_connection *mongo_get_connection_single(mongo_con_manager *manager, mongo_servers *servers, int server_id)
{
	char *hash;
	mongo_connection *con;

	hash = mongo_server_create_hash(servers->server[server_id]);
	con = mongo_manager_connection_find_by_hash(manager, hash);
	if (!con) {
		con = mongo_connection_create(servers->server[server_id]);
		if (con) {
			if (mongo_connection_ping(con)) {
				mongo_manager_connection_register(manager, hash, con);
			} else {
				mongo_connection_destroy(con);
				free(hash);
				return NULL;
			}
		}
	}
	free(hash);

	return con;
}

/* Topology discovery */

/* - Helpers */
static void mongo_discover_nodes(mongo_con_manager *manager, mongo_server_def *server)
{
	char *hash;
	mongo_connection *con;

	hash = mongo_server_create_hash(server);
	con = mongo_manager_connection_find_by_hash(manager, hash);

	if (mongo_connection_is_master(con)) {
	}

	free(hash);
}

static void mongo_discover_topology(mongo_con_manager *manager, mongo_servers *servers)
{
	int i;

	for (i = 0; i < servers->count; i++) {
		mongo_discover_nodes(manager, servers->server[i]);
	}
}

/* Fetching connections */
static mongo_connection *mongo_get_connection_standalone(mongo_con_manager *manager, mongo_servers *servers)
{
	return mongo_get_connection_single(manager, servers, 0);
}

static mongo_connection *mongo_get_connection_replicaset(mongo_con_manager *manager, mongo_servers *servers)
{
	mongo_connection *con;
	int i;

	/* Create a connection to every of the servers in the seed list */
	for (i = 0; i < servers->count; i++) {
		con = mongo_get_connection_single(manager, servers, i);
	}
	/* Discover more nodes */
	mongo_discover_topology(manager, servers);
	return con;
}

/* API interface to fetch a connection */
mongo_connection *mongo_get_connection(mongo_con_manager *manager, mongo_servers *servers)
{
	/* Which connection we return depends on the type of connection we want */
	switch (servers->con_type) {
		case MONGO_CON_TYPE_STANDALONE:
			return mongo_get_connection_standalone(manager, servers);

		case MONGO_CON_TYPE_REPLSET:
			return mongo_get_connection_replicaset(manager, servers);
/*
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
