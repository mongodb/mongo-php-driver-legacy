#ifndef __MCON_PARSE_H__
#define __MCON_PARSE_H__

#include "types.h"

#define MONGO_CON_TYPE_STANDALONE 1
#define MONGO_CON_TYPE_MULTIPLE   2
#define MONGO_CON_TYPE_REPLSET    3

/* Parsing server connection strings and its cleanup routines */
mongo_servers* mongo_parse_server_spec(char *spec);
void mongo_servers_dump(mongo_servers *servers);
void mongo_servers_dtor(mongo_servers *servers);
#endif
