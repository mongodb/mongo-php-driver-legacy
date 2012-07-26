#include "collection.h"
#include "types.h"
#include "read_preference.h"
#include <stdio.h>
#include <stdlib.h>

/* Helpers */
void mongo_print_connection_info(void *elem)
{
	mongo_connection *con = (mongo_connection*) elem;
	MCONDBG(printf("- connection: type: %s, socket: %d, ping: %d, hash: %s\n",
		con->connection_type == 1 ? "PRIMARY  " : "SECONDARY",
		con->socket,
		con->ping_ms,
		con->hash
	));
}

/* Collecting the correct servers */
static mcon_collection *filter_connections(mongo_con_manager *manager, int types)
{
	mcon_collection *col;
	mongo_con_manager_item *ptr = manager->connections;

	col = mcon_init_collection(sizeof(mongo_connection*));

	while (ptr) {
		/* we need to check for username and pw on the connection too later */
		if (ptr->connection->connection_type & types) {
			mongo_print_connection_info(ptr->connection);
			mcon_collection_add(col, ptr->connection);
		}
		ptr = ptr->next;
	}

	return col;
}

static mcon_collection *mongo_rp_collect_primary(mongo_con_manager *manager)
{
	return filter_connections(manager, MONGO_NODE_PRIMARY);
}

static mcon_collection *mongo_rp_collect_primary_and_secondary(mongo_con_manager *manager)
{
	return filter_connections(manager, MONGO_NODE_PRIMARY | MONGO_NODE_SECONDARY);
}

static mcon_collection *mongo_rp_collect_secondary(mongo_con_manager *manager)
{
	return filter_connections(manager, MONGO_NODE_SECONDARY);
}

mcon_collection* mongo_find_candidate_servers(mongo_con_manager *manager, mongo_read_preference *rp)
{
	MCONDBG(printf("finding candidate servers\n"));
	/* Depending on read preference type, run the correct algorithm */
	switch (rp->type) {
		case MONGO_RP_PRIMARY:
			return mongo_rp_collect_primary(manager);
			break;
		case MONGO_RP_PRIMARY_PREFERRED:
		case MONGO_RP_SECONDARY_PREFERRED:
		case MONGO_RP_NEAREST:
			return mongo_rp_collect_primary_and_secondary(manager);
			break;
		case MONGO_RP_SECONDARY:
			return mongo_rp_collect_secondary(manager);
			break;
		default:
			return NULL;
	}
}

/* Sorting the servers */
static int mongo_rp_sort_primary_preferred(const void* a, const void *b)
{
	mongo_connection *con_a = *(mongo_connection**) a;
	mongo_connection *con_b = *(mongo_connection**) b;

	/* First we prefer primary over secondary, and if the field type is the
	 * same, we sort on ping_ms again. *_SECONDARY is a higher constant value
	 * than *_PRIMARY, so we sort descendingly by connection_type */
	if (con_a->connection_type > con_b->connection_type) {
		return 1;
	} else if (con_a->connection_type < con_b->connection_type) {
		return -1;
	} else {
		if (con_a->ping_ms > con_b->ping_ms) {
			return 1;
		} else if (con_a->ping_ms < con_b->ping_ms) {
			return -1;
		}
	}
	return 0;
}

static int mongo_rp_sort_secondary_preferred(const void* a, const void *b)
{
	mongo_connection *con_a = *(mongo_connection**) a;
	mongo_connection *con_b = *(mongo_connection**) b;

	/* First we prefer secondary over primary, and if the field type is the
	 * same, we sort on ping_ms again. *_SECONDARY is a higher constant value
	 * than *_PRIMARY. */
	if (con_a->connection_type < con_b->connection_type) {
		return 1;
	} else if (con_a->connection_type > con_b->connection_type) {
		return -1;
	} else {
		if (con_a->ping_ms > con_b->ping_ms) {
			return 1;
		} else if (con_a->ping_ms < con_b->ping_ms) {
			return -1;
		}
	}
	return 0;
}

static int mongo_rp_sort_any(const void* a, const void *b)
{
	mongo_connection *con_a = *(mongo_connection**) a;
	mongo_connection *con_b = *(mongo_connection**) b;

	if (con_a->ping_ms > con_b->ping_ms) {
		return 1;
	} else if (con_a->ping_ms < con_b->ping_ms) {
		return -1;
	}
	return 0;
}

/* This method is the master for selecting the correct algorithm for the order
 * of servers in which to try the candidate servers that we've previously found */
mcon_collection *mongo_sort_servers(mcon_collection *col, mongo_read_preference *rp)
{
	mongo_connection_sort_t *sort_function;

	switch (rp->type) {
		case MONGO_RP_PRIMARY:
			/* Should not really have to do anything as there is only going to
			 * be one server */
			sort_function = mongo_rp_sort_any;
			break;

		case MONGO_RP_PRIMARY_PREFERRED:
			sort_function = mongo_rp_sort_primary_preferred;
			break;

		case MONGO_RP_SECONDARY:
			sort_function = mongo_rp_sort_any;
			break;

		case MONGO_RP_SECONDARY_PREFERRED:
			sort_function = mongo_rp_sort_secondary_preferred;
			break;

		case MONGO_RP_NEAREST:
			sort_function = mongo_rp_sort_any;
			break;

		default:
			return NULL;
	}
	MCONDBG(printf("select server: sorting\n"));
	qsort(col->data, col->count, sizeof(mongo_connection*), sort_function);
	mcon_collection_iterate(col, mongo_print_connection_info);
	return col;
}

mcon_collection *mongo_select_nearest_servers(mcon_collection *col, mongo_read_preference *rp)
{
	mcon_collection *filtered;
	int              i, nearest_ping;

	filtered = mcon_init_collection(sizeof(mongo_connection*));

	MCONDBG(printf("select server: only nearest\n"));

	switch (rp->type) {
		case MONGO_RP_PRIMARY:
		case MONGO_RP_PRIMARY_PREFERRED:
		case MONGO_RP_SECONDARY:
		case MONGO_RP_SECONDARY_PREFERRED:
		case MONGO_RP_NEAREST:
			/* The nearest ping time is in the first element */
			nearest_ping = ((mongo_connection*)col->data[0])->ping_ms;
			MCONDBG(printf("select server: nearest is %dms\n", nearest_ping));

			/* FIXME: Change to iterator later */
			for (i = 0; i < col->count; i++) {
				if (((mongo_connection*)col->data[i])->ping_ms <= nearest_ping + MONGO_RP_CUTOFF) {
					mcon_collection_add(filtered, col->data[i]);
				}
			}
			break;

		default:
			return NULL;
	}

	/* Clean up the old collection that we no longer need */
	mcon_collection_free(col);

	mcon_collection_iterate(filtered, mongo_print_connection_info);
	return filtered;
}

mongo_connection *mongo_pick_server_from_set(mcon_collection *col, mongo_read_preference *rp)
{
	int entry = rand() % col->count;

	if (rp->type == MONGO_RP_PRIMARY_PREFERRED) {
		if (((mongo_connection*)col->data[0])->connection_type == MONGO_NODE_PRIMARY) {
			MCONDBG(printf("pick server: the primary\n"));
			return (mongo_connection*)col->data[0];
		}
	}
	/* For now, we just pick a random server from the set */
	MCONDBG(printf("pick server: random element %d\n", entry));
	return (mongo_connection*)col->data[entry];
}
