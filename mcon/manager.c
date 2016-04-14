/**
 *  Copyright 2009-2014 MongoDB, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef WIN32
#define va_copy(s,d) ((void)((d) = (s)))
#else
#include <sys/time.h>
#endif

#include "types.h"
#include "utils.h"
#include "manager.h"
#include "connections.h"
#include "collection.h"
#include "parse.h"
#include "read_preference.h"
#include "contrib/strndup.h"

/* Forwards declarations */
static void mongo_blacklist_destroy(mongo_con_manager *manager, void *data, int why);

/* Helpers */
static mongo_connection *mongo_get_connection_single(mongo_con_manager *manager, mongo_server_def *server, mongo_server_options *options, int connection_flags, char **error_message)
{
	char *hash;
	mongo_connection *con = NULL;
	mongo_connection_blacklist *blacklist = NULL;

	hash = mongo_server_create_hash(server);

	/* See if a connection is in our blacklist to short-circut trying to
	 * connect to a node that is known to be down. This is done so we don't
	 * waste precious time in connecting to unreachable nodes */
	blacklist = mongo_manager_blacklist_find_by_hash(manager, hash);
	if (blacklist) {
		struct timeval start;
		/* It is blacklisted, but it may have been a long time again and
		 * chances are we should give it another try */
		if (mongo_connection_ping_check(manager, blacklist->last_ping, &start)) {
			/* The connection is blacklisted, but we've reached our ping
			 * interval so lets remove the blacklisting and pretend we didn't
			 * know about it */
			mongo_manager_blacklist_deregister(manager, blacklist, hash);
		} else {
			/* Otherwise short-circut the connection attempt, and say we failed
			 * right away */
			free(hash);
			*error_message = strdup("Previous connection attempts failed, server blacklisted");
			return NULL;
		}
	}

	con = mongo_manager_connection_find_by_hash(manager, hash);

	/* If we aren't about to (re-)connect then all we care about if it was a
	 * known connection or not */
	if (connection_flags & MONGO_CON_FLAG_DONT_CONNECT) {
		free(hash);
		return con;
	}

	/* If we found a valid connection check if we need to ping it */
	if (con) {
		/* Do the ping, if needed */
		if (!mongo_connection_ping(manager,  con, options, error_message)) {
			/* If the ping failed, deregister the connection */
			mongo_manager_connection_deregister(manager, con);
			/* Set the return value to NULL, as the connection is broken and
			 * has been removed */
			con = NULL;
		}

		free(hash);
		return con;
	}

	/* Since we didn't find an existing connection, lets make one! */
	con = mongo_connection_create(manager, hash, server, options, error_message);
	if (con) {
		/* isMaster() _must_ be the first command on all new connections.
		 * This is for node discovery so we don't issue f.e. authentication to nodes in STARTUP
		 * state, or arbiters */
		if (!mongo_connection_ismaster(manager, con, options, NULL, 0, NULL, error_message, NULL)) {
			mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "ismaster: error running ismaster: %s",  *error_message);
			mongo_connection_destroy(manager, con, MONGO_CLOSE_BROKEN);
			free(hash);
			return NULL;
		}

		/* When we make a connection, we need to figure out the server version it is */
		if (!mongo_connection_get_server_version(manager, con, options, error_message)) {
			mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "server_version: error while getting the server version %s:%d: %s", server->host, server->port, *error_message);
			mongo_connection_destroy(manager, con, MONGO_CLOSE_BROKEN);
			free(hash);
			return NULL;
		}

		/* Do authentication if requested */
		/* Note: Arbiters don't contain any data, including auth stuff, so you cannot authenticate on an arbiter */
		if (con->connection_type != MONGO_NODE_ARBITER) {
			if (!manager->authenticate(manager, con, options, server, error_message)) {
				mongo_connection_destroy(manager, con, MONGO_CLOSE_BROKEN);
				free(hash);
				return NULL;
			}
		}

		/* Do the first-time ping to record the latency of the connection */
		if (mongo_connection_ping(manager, con, options, error_message)) {
			/* Register the connection on successful pinging */
			mongo_manager_connection_register(manager, con);
		} else {
			/* Or kill it and reset the return value if the ping somehow failed */
			mongo_connection_destroy(manager, con, MONGO_CLOSE_BROKEN);
			con = NULL;
		}
	}

	free(hash);

	if (con) {
		con->connected = 1;
	}

	return con;
}

static int mongo_strings_equal_or_null(const char *s1, const char *s2)
{
	return (
		(s1 == NULL && s2 == NULL) ||
		(s1 != NULL && s2 != NULL && strcmp(s1, s2) == 0)
	);
}

static int mongo_servers_contains(const mongo_servers *servers, const mongo_server_def *def)
{
	int i;

	for (i = 0; i < servers->count; i++) {
		mongo_server_def *tmp = servers->server[i];

		if (
			tmp->mechanism == def->mechanism &&
			tmp->port == def->port &&
			strcmp(tmp->host, def->host) == 0 &&
			/* Compare optional strings, which may be NULL */
			mongo_strings_equal_or_null(tmp->repl_set_name, def->repl_set_name) && 
			mongo_strings_equal_or_null(tmp->db, def->db) && 
			mongo_strings_equal_or_null(tmp->authdb, def->authdb) && 
			mongo_strings_equal_or_null(tmp->username, def->username) && 
			mongo_strings_equal_or_null(tmp->password, def->password)
		) {
			return 1;
		}
	}

	return 0;
}

/* Topology discovery */

/* - Helpers */
/* Returns:
 * 1 on success
 * 0 on total failure (e.g. unsupported wire version) */
static int mongo_discover_topology(mongo_con_manager *manager, mongo_servers *servers)
{
	int i, j;
	char *hash;
	mongo_connection *con;
	char *error_message;
	char *repl_set_name = servers->options.repl_set_name ? strdup(servers->options.repl_set_name) : NULL;
	int nr_hosts;
	char **found_hosts = NULL;
	char *tmp_hash;
	int   res;
	int found_supported_wire_version = 1; /* Innocent unless proven guilty */

	for (i = 0; i < servers->count; i++) {
		hash = mongo_server_create_hash(servers->server[i]);
		mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "discover_topology: checking ismaster for %s", hash);
		con = mongo_manager_connection_find_by_hash(manager, hash);

		if (!con) {
			mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "discover_topology: couldn't create a connection for %s", hash);
			free(hash);
			continue;
		}
		
		/* Run ismaster, if needed, to extract server flags - and fetch the other known hosts */
		res = mongo_connection_ismaster(manager, con, &servers->options, (char**) &repl_set_name, (int*) &nr_hosts, (char***) &found_hosts, (char**) &error_message, servers->server[i]);
		switch (res) {
			case 4:
				/* The server is running unsupported wire versions */
				found_supported_wire_version = 0;
				/* break omitted intentionally */
			case 0:
				/* Something is wrong with the connection, we need to remove
				 * this from our list */
				mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "discover_topology: ismaster return with an error for %s:%d: [%s]", servers->server[i]->host, servers->server[i]->port, error_message);
				free(error_message);
				mongo_manager_connection_deregister(manager, con);
				break;

			case 3:
				mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "discover_topology: ismaster worked, but we need to remove the seed host's connection");
				mongo_manager_connection_deregister(manager, con);
				/* Break intentionally missing */

			case 1:
				mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "discover_topology: ismaster worked");
				/* Update the replica set name in the parsed "servers" struct
				 * so that we can consistently compare it to the information
				 * that is stored in the connection hashes. */
				if (!servers->options.repl_set_name && repl_set_name) {
					servers->options.repl_set_name = strdup(repl_set_name);
				}

				/* Now loop over all the hosts that were found */
				for (j = 0; j < nr_hosts; j++) {
					mongo_server_def *tmp_def;
					mongo_connection *new_con;
					char *con_error_message = NULL;

					/* Create a temp server definition to create a new
					 * connection on-demand if we didn't have one already */
					tmp_def = calloc(1, sizeof(mongo_server_def));
					tmp_def->username = servers->server[i]->username ? strdup(servers->server[i]->username) : NULL;
					tmp_def->password = servers->server[i]->password ? strdup(servers->server[i]->password) : NULL;
					tmp_def->repl_set_name = servers->server[i]->repl_set_name ? strdup(servers->server[i]->repl_set_name) : NULL;
					tmp_def->db = servers->server[i]->db ? strdup(servers->server[i]->db) : NULL;
					tmp_def->authdb = servers->server[i]->authdb ? strdup(servers->server[i]->authdb) : NULL;
					tmp_def->host = mcon_strndup(found_hosts[j], strchr(found_hosts[j], ':') - found_hosts[j]);
					tmp_def->port = atoi(strchr(found_hosts[j], ':') + 1);
					tmp_def->mechanism = servers->server[i]->mechanism;
					
					/* Create a hash so that we can check whether we already have a
					 * connection for this server definition. If we don't create
					 * the connection, register it (done in
					 * mongo_get_connection_single) and add it to the list of
					 * servers that we're processing so we might use this host to
					 * find more servers. */
					tmp_hash = mongo_server_create_hash(tmp_def);
					if (!mongo_manager_connection_find_by_hash(manager, tmp_hash)) {
						mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "discover_topology: found new host: %s:%d", tmp_def->host, tmp_def->port);
						new_con = mongo_get_connection_single(manager, tmp_def, &servers->options, MONGO_CON_FLAG_WRITE, (char **) &con_error_message);

						if (new_con) {
							int ismaster_error = mongo_connection_ismaster(manager, new_con, &servers->options, NULL, NULL, NULL, &con_error_message, NULL);

							switch (ismaster_error) {
								case 1: /* Run just fine */
								case 2: /* ismaster() skipped due to interval */
									break;

								case 4: /* Danger danger, reported wire version does not overlap what we support */
									found_supported_wire_version = 0;
									/* break omitted intentionally */

								case 0: /* Some error */
								case 3: /* Run just fine, but hostname didn't match what we expected */
								default:

									mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "server_flags: error while getting the server configuration %s:%d: %s", servers->server[i]->host, servers->server[i]->port, con_error_message);
									mongo_manager_connection_deregister(manager, new_con);
									new_con = NULL;
							}
						} else {
							mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "discover_topology: could not connect to new host: %s:%d: %s", tmp_def->host, tmp_def->port, con_error_message);
							free(con_error_message);
						}
					}

					if (!mongo_servers_contains(servers, tmp_def)) {
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
				break;

			case 2:
				mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "discover_topology: ismaster got skipped");
				break;
		}

		free(hash);
	}
	if (repl_set_name) {
		free(repl_set_name);
	}

	return found_supported_wire_version;
}

static mongo_connection *mongo_get_read_write_connection_replicaset(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, char **error_message)
{
	mongo_connection *con = NULL;
	mongo_connection *tmp;
	mcon_collection  *collection;
	char             *con_error_message = NULL;
	int i;
	int found_connected_server = 0;

	/* Create a connection to all of the servers in the seed list */
	for (i = 0; i < servers->count; i++) {
		tmp = mongo_get_connection_single(manager, servers->server[i], &servers->options, connection_flags, (char **) &con_error_message);

		if (tmp) {
			found_connected_server++;
		} else if (!(connection_flags & MONGO_CON_FLAG_DONT_CONNECT)) {
			mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Couldn't connect to '%s:%d': %s", servers->server[i]->host, servers->server[i]->port, con_error_message);
			free(con_error_message);
		}
	}
	if (!found_connected_server && (connection_flags & MONGO_CON_FLAG_DONT_CONNECT)) {
		return NULL;
	}

	/* Discover more nodes. This also adds a connection to "servers" for each
	 * new node */
	if (!mongo_discover_topology(manager, servers)) {
		/* Total failure, we cannot proceed */
		*error_message = strdup("Incompatible server detected. This driver release is not compatible with one of the connected servers");
		return NULL;
	}

	/* Depending on whether we want a read or a write connection, run the correct algorithms */
	if (connection_flags & MONGO_CON_FLAG_WRITE) {
		mongo_read_preference tmp_rp;

		tmp_rp.type = MONGO_RP_PRIMARY;
		tmp_rp.tagsets = NULL;
		tmp_rp.tagset_count = 0;

		collection = mongo_find_candidate_servers(manager, &tmp_rp, servers);
		mongo_read_preference_dtor(&tmp_rp);
	} else if (connection_flags & MONGO_CON_FLAG_DONT_FILTER) {
		/* We just want to know if we have something to talk to, irregardless of RP */
		mongo_read_preference tmp_rp;

		tmp_rp.type = MONGO_RP_ANY;
		tmp_rp.tagsets = NULL;
		tmp_rp.tagset_count = 0;

		collection = mongo_find_candidate_servers(manager, &tmp_rp, servers);
		mongo_read_preference_dtor(&tmp_rp);
	} else {
		collection = mongo_find_candidate_servers(manager, &servers->read_pref, servers);
	}
	if (!collection) {
		*error_message = strdup("No candidate servers found");
		return NULL;
	}
	if (collection->count == 0) {
		*error_message = strdup("No candidate servers found");
		mcon_collection_free(collection);	
		return NULL;
	}
	collection = mongo_sort_servers(manager, collection, &servers->read_pref);
	collection = mongo_select_nearest_servers(manager, collection, &servers->options, &servers->read_pref);
	con = mongo_pick_server_from_set(manager, collection, &servers->read_pref);

	/* Cleaning up */
	mcon_collection_free(collection);	
	return con;
}


static mongo_connection *mongo_get_connection_multiple(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, char **error_message)
{
	mongo_connection *con = NULL;
	mongo_connection *tmp;
	mcon_collection  *collection = NULL;
	mongo_read_preference tmp_rp; /* We only support NEAREST for MULTIPLE right now */
	int i;
	int found_connected_server = 0;
	mcon_str         *messages;
	int found_supported_wire_version = 1;

	mcon_str_ptr_init(messages);

	/* Create a connection to every of the servers in the seed list */
	for (i = 0; i < servers->count; i++) {
		int ismaster_error = 0;
		char *con_error_message = NULL;
		tmp = mongo_get_connection_single(manager, servers->server[i], &servers->options, connection_flags, (char **) &con_error_message);

		if (tmp) {
			found_connected_server++;

			/* Run ismaster, if needed, to extract server flags */
			ismaster_error = mongo_connection_ismaster(manager, tmp, &servers->options, NULL, NULL, NULL, &con_error_message, NULL);
			switch (ismaster_error) {
				case 1: /* Run just fine */
				case 2: /* ismaster() skipped due to interval */
					break;

				case 0: /* Some error */
				case 3: /* Run just fine, but hostname didn't match what we expected */
				case 4: /* Danger danger, reported wire version does not overlap what we support */
				default:
					mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "server_flags: error while getting the server configuration %s:%d: %s", servers->server[i]->host, servers->server[i]->port, con_error_message);
					/* If it failed because of wire version, we have to bail out completely
					 * later on, but we should continue to aggregate the errors in case more
					 * servers are unsupported */
					if (ismaster_error == 4) {
						found_supported_wire_version = 0;
					}
					mongo_manager_connection_deregister(manager, tmp);
					tmp = NULL;
					found_connected_server--;
			}
		}
		if (!tmp) {
			if (!(connection_flags & MONGO_CON_FLAG_DONT_CONNECT)) {
				mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "Couldn't connect to '%s:%d': %s", servers->server[i]->host, servers->server[i]->port, con_error_message);
				if (messages->l) {
					mcon_str_addl(messages, "; ", 2, 0);
				}
				mcon_str_add(messages, "Failed to connect to: ", 0);
				mcon_str_add(messages, servers->server[i]->host, 0);
				mcon_str_addl(messages, ":", 1, 0);
				mcon_str_add_int(messages, servers->server[i]->port);
				mcon_str_addl(messages, ": ", 2, 0);
				mcon_str_add(messages, con_error_message, 1); /* Also frees con_error_message */
			} else {
				free(con_error_message);
			}
		}
	}

	if (!found_supported_wire_version) {
		*error_message = strdup("Found a server running unsupported wire version. Please upgrade the driver, or take the server out of rotation");
		mcon_str_ptr_dtor(messages);
		return NULL;
	}

	/* If we don't have a connected server then there is no point in continueing */
	if (!found_connected_server && (connection_flags & MONGO_CON_FLAG_DONT_CONNECT)) {
		mcon_str_ptr_dtor(messages);
		return NULL;
	}

	/* When selecting a *standalone, *mongos* or *arbiter* node,
	 * readPreferences make no sense as we don't have a "primary" or
	 * "secondary". The mongos nodes aren't tagged either.  To pick a node we
	 * therefore simply pick any one of our nodes.*/
	tmp_rp.type = MONGO_RP_ANY;
	tmp_rp.tagsets = NULL;
	tmp_rp.tagset_count = 0;
	collection = mongo_find_candidate_servers(manager, &tmp_rp, servers);
	if (!collection || collection->count == 0) {
		if (messages->l) {
			*error_message = strdup(messages->d);
		} else {
			*error_message = strdup("No candidate servers found");
		}
		goto bailout;
	}
	collection = mongo_sort_servers(manager, collection, &servers->read_pref);
	collection = mongo_select_nearest_servers(manager, collection, &servers->options, &servers->read_pref);
	if (!collection) {
		*error_message = strdup("No server near us");
		goto bailout;
	}
	con = mongo_pick_server_from_set(manager, collection, &servers->read_pref);

bailout:
	/* Cleaning up */
	mcon_str_ptr_dtor(messages);
	if (collection) {
		mcon_collection_free(collection);	
	}
	return con;
}

int mongo_deregister_callback_from_connection(mongo_connection *connection, void *cursor)
{
	if (connection->cleanup_list) {
		mongo_connection_deregister_callback *ptr = connection->cleanup_list;
		mongo_connection_deregister_callback *prev = NULL;
		do {
			if (ptr && ptr->callback_data == cursor) {
				if (prev) {
					prev->next = ptr->next;
				} else {
					connection->cleanup_list = ptr->next;
				}
				free(ptr);
				ptr = NULL;
				break;
			}
			prev = ptr;
			ptr = ptr->next;
		} while (ptr);
	}
	return 1;

}

/* API interface to fetch a connection */
mongo_connection *mongo_get_read_write_connection(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, char **error_message)
{
	/* In some cases we won't actually have a manager or servers initialized, for example when extending PHP objects without calling the constructor,
	 * and then var_dump() it or access the properties, for example the "connected" property */
	if (!manager || !servers) {
		return NULL;
	}

	/* Which connection we return depends on the type of connection we want */
	switch (servers->options.con_type) {
		case MONGO_CON_TYPE_STANDALONE:
			mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "mongo_get_read_write_connection: finding a STANDALONE connection");
			return mongo_get_connection_multiple(manager, servers, connection_flags, error_message);

		case MONGO_CON_TYPE_REPLSET:
			mongo_manager_log(
				manager, MLOG_CON, MLOG_INFO,
				"mongo_get_read_write_connection: finding a REPLSET connection (%s)",
				connection_flags & MONGO_CON_FLAG_WRITE ? "write" : "read"
			);
			return mongo_get_read_write_connection_replicaset(manager, servers, connection_flags, error_message);

		case MONGO_CON_TYPE_MULTIPLE:
			mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "mongo_get_read_write_connection: finding a MULTIPLE connection");
			return mongo_get_connection_multiple(manager, servers, connection_flags, error_message);

		default:
			mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "mongo_get_read_write_connection: connection type %d is not supported", servers->options.con_type);
			*error_message = strdup("mongo_get_read_write_connection: Unknown connection type requested");
	}
	return NULL;
}

mongo_connection *mongo_manager_add_connection_callback(mongo_connection *connection, void *callback_data, mongo_cleanup_t cleanup_cb)
{
	mongo_connection_deregister_callback *cb;

	cb = calloc(1, sizeof(mongo_connection_deregister_callback));

	cb->mongo_cleanup_cb = cleanup_cb;
	cb->callback_data = callback_data;

	if (connection->cleanup_list) {
		mongo_connection_deregister_callback *ptr = connection->cleanup_list;
		do {
			if (!ptr->next) {
				ptr->next = cb;
				break;
			}
			ptr = ptr->next;
		} while (1);
	} else {
		connection->cleanup_list = cb;
	}
	return connection;
}
mongo_connection *mongo_get_read_write_connection_with_callback(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, void *callback_data, mongo_cleanup_t cleanup_cb, char **error_message)
{
	mongo_connection *connection;

	connection = mongo_get_read_write_connection(manager, servers, connection_flags, error_message);

	if (!connection) {
		return NULL;
	}

	return mongo_manager_add_connection_callback(connection, callback_data, cleanup_cb);
}

/* Connection management */

/* - Helpers */
static mongo_con_manager_item *create_new_manager_item(void)
{
	mongo_con_manager_item *tmp = malloc(sizeof(mongo_con_manager_item));
	memset(tmp, 0, sizeof(mongo_con_manager_item));

	return tmp;
}

static void free_manager_item(mongo_con_manager *manager, mongo_con_manager_item *item)
{
	mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "freeing connection %s", item->hash);
	free(item->hash);
	free(item);
}

static void destroy_manager_item(mongo_con_manager *manager, mongo_con_manager_item *item, mongo_con_manager_item_destroy_t cleanup_cb)
{
	if (item->next) {
		destroy_manager_item(manager, item->next, cleanup_cb);
	}
	cleanup_cb(manager, item->data, MONGO_CLOSE_SHUTDOWN);
	free_manager_item(manager, item);
}

void *mongo_manager_find_by_hash(mongo_con_manager *manager, mongo_con_manager_item *ptr, char *hash)
{
	while (ptr) {
		if (strcmp(ptr->hash, hash) == 0) {
			mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "found connection %s (looking for %s)", ptr->hash, hash);
			return ptr->data;
		}
		ptr = ptr->next;
	}
	return NULL;
}

mongo_connection *mongo_manager_connection_find_by_server_definition(mongo_con_manager *manager, mongo_server_def *definition)
{
	char *hash = mongo_server_create_hash(definition);
	mongo_connection *con = mongo_manager_find_by_hash(manager, manager->connections, hash);

	free(hash);
	return con;
}

mongo_connection *mongo_manager_connection_find_by_hash_with_callback(mongo_con_manager *manager, char *hash, void *callback_data, mongo_cleanup_t cleanup_cb)
{
	mongo_connection *connection;

	connection = mongo_manager_find_by_hash(manager, manager->connections, hash);
	return mongo_manager_add_connection_callback(connection, callback_data, cleanup_cb);
}
mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash)
{
	return mongo_manager_find_by_hash(manager, manager->connections, hash);
}

mongo_connection_blacklist *mongo_manager_blacklist_find_by_hash(mongo_con_manager *manager, char *hash)
{
	return mongo_manager_find_by_hash(manager, manager->blacklist, hash);
}

mongo_con_manager_item *mongo_manager_register(mongo_con_manager *manager, mongo_con_manager_item **ptr, void *con, char *hash)
{
	mongo_con_manager_item *new;

	/* Setup new entry */
	new = create_new_manager_item();
	new->data = con;
	new->hash = strdup(hash);
	new->next = NULL;

	if (!*ptr) { /* No connections at all yet */
		*ptr = new;
	} else {
		mongo_con_manager_item *item = *ptr;
		/* Existing connections, so find the last one */
		do {
			if (!item->next) {
				item->next = new;
				break;
			}
			item = item->next;
		} while (1);
	}
	return new;
}

void mongo_manager_connection_register(mongo_con_manager *manager, mongo_connection *con)
{
	mongo_manager_register(manager, &manager->connections, con, con->hash);
}

void mongo_manager_blacklist_register(mongo_con_manager *manager, mongo_connection *data)
{
	struct timeval start;
	mongo_connection_blacklist *blacklist;

	blacklist = malloc(sizeof(mongo_connection_blacklist));
	memset(blacklist, 0, sizeof(mongo_connection_blacklist));
	gettimeofday(&start, NULL);
	blacklist->last_ping = start.tv_sec;
	mongo_manager_register(manager, &manager->blacklist, blacklist, data->hash);
}

int mongo_manager_deregister(mongo_con_manager *manager, mongo_con_manager_item **ptr, char *hash, void *con, mongo_con_manager_item_destroy_t cleanup_cb)
{
	mongo_con_manager_item *prev = NULL;
	mongo_con_manager_item *item = *ptr;

	/* Remove from manager */
	/* - if there are no connections, simply return false */
	if (!item) {
		return 0;
	}
	/* Loop over existing connections and compare hashes */
	do {
		/* The connection is the one we're looking for */
		if (strcmp((item)->hash, hash) == 0) {
			if (prev == NULL) { /* It's the first one in the list... */
				*ptr = item->next;
			} else {
				prev->next = item->next;
			}
			/* Free structures */
			if (cleanup_cb) {
				cleanup_cb(manager, con, MONGO_CLOSE_BROKEN);
			}
			free(item->hash);
			free(item);

			/* Woo! */
			return 1;
		}

		/* Next iteration */
		prev = item;
		item = item->next;
	} while (item);

	return 0;
}

int mongo_manager_connection_deregister(mongo_con_manager *manager, mongo_connection *con)
{
	return mongo_manager_deregister(manager, &manager->connections, con->hash, con, mongo_connection_destroy);
}

int mongo_manager_blacklist_deregister(mongo_con_manager *manager, mongo_connection_blacklist *blacklist_item, char *hash)
{
	return mongo_manager_deregister(manager, &manager->blacklist, hash, blacklist_item, mongo_blacklist_destroy);
}

/* Logging */
void mongo_manager_log(mongo_con_manager *manager, int module, int level, char *format, ...)
{
	va_list arg;

	va_start(arg, format);
	if (manager->log_function) {
		manager->log_function(module, level, manager->log_context, format, arg);
	}
	va_end(arg);
}

/* Log handler which does nothing */
void mongo_log_null(int module, int level, void *context, char *format, va_list arg)
{
}

/* Log handler which uses printf */
void mongo_log_printf(int module, int level, void *context, char *format, va_list arg)
{
	va_list  tmp;
	char    *message;

	message = malloc(1024);

	va_copy(tmp, arg);
	vsnprintf(message, 1024, format, tmp);
	va_end(tmp);

	printf("%s\n", message);
	free(message);
}

/* Init/deinit */
mongo_con_manager *mongo_init(void)
{
	mongo_con_manager *tmp;

	tmp = malloc(sizeof(mongo_con_manager));
	memset(tmp, 0, sizeof(mongo_con_manager));

	tmp->log_context = NULL;
	tmp->log_function = mongo_log_null;

	tmp->ping_interval = MONGO_MANAGER_DEFAULT_PING_INTERVAL;
	tmp->ismaster_interval = MONGO_MANAGER_DEFAULT_MASTER_INTERVAL;

	tmp->connect               = NULL;
	tmp->recv_header           = NULL;
	tmp->recv_data             = NULL;
	tmp->send                  = NULL;
	tmp->close                 = NULL;
	tmp->forget                = NULL;
	tmp->authenticate          = NULL;
	tmp->supports_wire_version = NULL;

	return tmp;
}

static void mongo_blacklist_destroy(mongo_con_manager *manager, void *data, int why)
{
	mongo_connection_blacklist *con = (mongo_connection_blacklist *)data;
	free(con);
}

void mongo_deinit(mongo_con_manager *manager)
{
	if (manager->connections) {
		/* Does this iteratively for all connections */
		destroy_manager_item(manager, manager->connections, mongo_connection_destroy);
	}
	if (manager->blacklist) {
		/* Does this iteratively for all blacklist items */
		destroy_manager_item(manager, manager->blacklist, mongo_blacklist_destroy);
	}

	free(manager);
}

int mongo_mcon_supports_wire_version(int min_wire_version, int max_wire_version, char **error_message)
{
	char *errmsg = "This driver version requires WireVersion between minWireVersion: %d and maxWireVersion: %d. Got: minWireVersion=%d and maxWireVersion=%d";
	int errlen = strlen(errmsg) - 8 + 1 + (4 * 10); /* Subtract the %d, plus \0, plus 4 ints at maximum size.. */

	if (min_wire_version > MCON_MAX_WIRE_VERSION) {
		*error_message = malloc(errlen);
		snprintf(*error_message, errlen, errmsg, MCON_MIN_WIRE_VERSION, MCON_MAX_WIRE_VERSION, min_wire_version, max_wire_version);
		return 0;
	}

	if (max_wire_version < MCON_MIN_WIRE_VERSION) {
		*error_message = malloc(errlen);
		snprintf(*error_message, errlen, errmsg, MCON_MIN_WIRE_VERSION, MCON_MAX_WIRE_VERSION, min_wire_version, max_wire_version);
		return 0;
	}

	return 1;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
