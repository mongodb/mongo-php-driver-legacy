#ifndef __MCON_PARSE_H__
#define __MCON_PARSE_H__

#include "types.h"

/* Parsing server connection strings and its cleanup routines */
mongo_servers* mongo_parse_init(void);
mongo_servers* mongo_parse_server_spec(mongo_con_manager *manager, mongo_servers *servers, char *spec);
void mongo_servers_dump(mongo_con_manager *manager, mongo_servers *servers);
void mongo_server_def_dtor(mongo_server_def *server_def);
void mongo_servers_dtor(mongo_servers *servers);
#endif
