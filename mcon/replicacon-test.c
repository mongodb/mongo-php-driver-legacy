#include "parse.h"
#include "manager.h"
#include <stdio.h>

int main(void)
{
	mongo_servers *servers;
	mongo_con_manager *manager;
	char *error_message;

	manager = mongo_init();

//	servers = mongo_parse_server_spec("mongodb://root:root@127.0.0.1:13002,127.0.0.1:27017,localhost:13001/demo?replicaSet=seta");
	servers = mongo_parse_server_spec(manager, "mongodb://whisky:13002,whisky:13000,whisky:13001/demo?replicaSet=seta");
	mongo_servers_dump(manager, servers);
	servers->rp.type = MONGO_RP_PRIMARY_PREFERRED;
	mongo_get_read_write_connection(manager, servers, 0, (char**) &error_message);
	if (error_message) {
		printf("ERROR: %s\n", error_message);
	}
	mongo_servers_dtor(servers);

	mongo_deinit(manager);

	return 0;
}
