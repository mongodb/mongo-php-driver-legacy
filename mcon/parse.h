#define MONGO_CON_TYPE_STANDALONE 1
#define MONGO_CON_TYPE_MULTIPLE   2
#define MONGO_CON_TYPE_REPLSET    3

/* Parsing server connection strings and its cleanup routines */
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

mongo_servers* mongo_parse_server_spec(char *spec);
void mongo_servers_dump(mongo_servers *servers);
void mongo_servers_dtor(mongo_servers *servers);
