#ifndef __MCON_TYPES_H__
#define __MCON_TYPES_H__

#include <time.h>

#define MONGO_CON_TYPE_STANDALONE 1
#define MONGO_CON_TYPE_MULTIPLE   2
#define MONGO_CON_TYPE_REPLSET    3

typedef struct _mongo_connection
{
	time_t last_ping;
	int    ping_ms;
} mongo_connection;

typedef struct _mongo_con_manager
{
	int dummy;
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
