#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "utils.h"
#include "manager.h"
#include "connections.h"

/* Helpers */
static mongo_connection *mongo_get_connection_single(mongo_con_manager *manager, mongo_server_def *server)
{
	char *hash;
	mongo_connection *con;

	hash = mongo_server_create_hash(server);
	con = mongo_manager_connection_find_by_hash(manager, hash);
	if (!con) {
		con = mongo_connection_create(server);
		if (con) {
			printf("get_connection_single: pinging %s\n", hash);
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
static void mongo_discover_topology(mongo_con_manager *manager, mongo_servers *servers)
{
	int i, j;
	char *hash;
	mongo_connection *con;
	char *error_message;
	char *repl_set_name = servers->repl_set_name ? strdup(servers->repl_set_name) : NULL;
	int nr_hosts;
	char **found_hosts = NULL;
	char *tmp_hash;

	for (i = 0; i < servers->count; i++) {
		hash = mongo_server_create_hash(servers->server[i]);
		printf("discover_topology: checking is_master for %s\n", hash);
		con = mongo_manager_connection_find_by_hash(manager, hash);

		if (!con) {
			printf("discover_topology: couldn't create a connection for %s\n", hash);
			free(hash);
			continue;
		}
		if (mongo_connection_is_master(con, (char**) &repl_set_name, (int*) &nr_hosts, (char***) &found_hosts, (char**) &error_message)) {
			printf("discover_topology: is_master worked\n");
			for (j = 0; j < nr_hosts; j++) {
				mongo_server_def *tmp_def;
				mongo_connection *new_con;

				/* Create a temp server definition to create a new connection */
				tmp_def = malloc(sizeof(mongo_server_def));
				tmp_def->username = servers->server[i]->username ? strdup(servers->server[i]->username) : NULL;
				tmp_def->password = servers->server[i]->password ? strdup(servers->server[i]->password) : NULL;
				tmp_def->db = servers->server[i]->db ? strdup(servers->server[i]->db) : NULL;
				tmp_def->host = strndup(found_hosts[j], strchr(found_hosts[j], ':') - found_hosts[j]);
				tmp_def->port = atoi(strchr(found_hosts[j], ':') + 1);
				
				/* Create a hash so that we can check whether we already have a
				 * connection for this server definition. If we don't create
				 * the connection, register it (done in
				 * mongo_get_connection_single) and add it to the list of
				 * servers that we're processing so we might use this host to
				 * find more servers. */
				tmp_hash = mongo_server_create_hash(tmp_def);
				if (!mongo_manager_connection_find_by_hash(manager, tmp_hash)) {
					printf("discover_topology: found new host: %s:%d\n", tmp_def->host, tmp_def->port);
					new_con = mongo_get_connection_single(manager, tmp_def);
					servers->server[servers->count] = tmp_def;
					servers->count++;
				} else {
					mongo_server_def_dtor(tmp_def);
				}
				free(tmp_hash);

				/* Cleanup */
				free(found_hosts[j]);
			}
			free(found_hosts);
			found_hosts = NULL;
		} else {
			/* Something is wrong with the connection, we need to remove
			 * this from our list */
			printf("discover_topology: is_master return with an error for %s:%d: [%s]\n", servers->server[i]->host, servers->server[i]->port, error_message);
			free(error_message);
			mongo_manager_connection_deregister(manager, hash, con);
		}

		free(hash);
	}
	if (repl_set_name) {
		free(repl_set_name);
	}
}

/* Fetching connections */
static mongo_connection *mongo_get_connection_standalone(mongo_con_manager *manager, mongo_servers *servers)
{
	return mongo_get_connection_single(manager, servers->server[0]);
}

static mongo_connection *mongo_get_connection_replicaset(mongo_con_manager *manager, mongo_servers *servers)
{
	mongo_connection *con;
	int i;

	/* Create a connection to every of the servers in the seed list */
	for (i = 0; i < servers->count; i++) {
		con = mongo_get_connection_single(manager, servers->server[i]);
	}
	/* Discover more nodes. This also adds a connection to "servers" for each
	 * new node */
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
		if (strcmp(ptr->hash, hash) == 0) {
			printf("found connection %s (looking for %s)\n", ptr->hash, hash);
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

static inline void free_manager_item(mongo_con_manager_item *item)
{
	printf("freeing connection %s\n", item->hash);
	free(item->hash);
	free(item);
}

static void destroy_manager_item(mongo_con_manager_item *item)
{
	if (item->next) {
		destroy_manager_item(item->next);
	}
	mongo_connection_destroy(item->connection);
	free_manager_item(item);
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

int mongo_manager_connection_deregister(mongo_con_manager *manager, char *hash, mongo_connection *con)
{
	mongo_con_manager_item *ptr = manager->connections;
	mongo_con_manager_item *prev = NULL;

	/* Remove from manager */
	/* - if there are no connections, simply return false */
	if (!ptr) {
		return 0;
	}
	/* Loop over existing connections and compare hashes */
	do {
		/* The connection is the one we're looking for */
		if (strcmp(ptr->hash, hash) == 0) {
			if (prev == NULL) { /* It's the first one in the list... */
				manager->connections = ptr->next;
			} else {
				prev->next = ptr->next;
			}
			/* Free structures */
			mongo_connection_destroy(con);
			free_manager_item(ptr);

			/* Woo! */
			return 1;
		}

		/* Next iteration */
		prev = ptr;
		ptr = ptr->next;
	} while (ptr);

	return 0;
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
