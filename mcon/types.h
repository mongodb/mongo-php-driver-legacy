#ifndef __MCON_TYPES_H__
#define __MCON_TYPES_H__

#include <time.h>

#define MONGO_CON_TYPE_STANDALONE 1
#define MONGO_CON_TYPE_MULTIPLE   2
#define MONGO_CON_TYPE_REPLSET    3

/* These constants are a bit field - however, each connection will only have
 * one type. The reason why it's a bit field is because of filtering during
 * read preference scanning (see read_preference.c).
 *
 * SECONDARY needs to have a larger constant value than PRIMARY for the read
 * preference sorting algorithm to work. */
#define MONGO_NODE_PRIMARY        0x01
#define MONGO_NODE_SECONDARY      0x02
#define MONGO_NODE_ARBITER        0x04
#define MONGO_NODE_MONGOS         0x08

typedef struct _mongo_connection
{
	time_t last_ping;
	int    ping_ms;
	int    last_reqid;
	int    socket;
	int    connection_type; /* MONGO_NODE_: PRIMARY, SECONDARY, ARBITER, MONGOS */
} mongo_connection;

typedef struct _mongo_con_manager_item
{
	char                           *hash;
	mongo_connection               *connection;
	struct _mongo_con_manager_item *next;
} mongo_con_manager_item;

typedef struct _mongo_con_manager
{
	mongo_con_manager_item *connections;
} mongo_con_manager;

typedef struct _mongo_server_def
{
	char *host;
	int   port;
	char *db;
	char *username;
	char *password;
} mongo_server_def;

typedef struct _mongo_servers
{
	int               count;
	mongo_server_def *server[16];

	/* flags and options */
	int               con_type;
	char             *repl_set_name;
} mongo_servers;
#endif
