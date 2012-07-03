#include "parse.h"
#include "manager.h"

int main(void)
{
	mongo_servers *servers;
	mongo_con_manager *manager;
	mongo_connection *con;
	mongo_read_preference rp;

	manager = mongo_init();

	servers = mongo_parse_server_spec("mongodb://root:root@127.0.0.1:13002,127.0.0.1:27017,localhost:13001/demo?replicaSet=seta");
	mongo_servers_dump(servers);
	rp.type = MONGO_RP_ANY;
	con = mongo_get_connection(manager, servers, &rp);
	mongo_servers_dtor(servers);

	mongo_deinit(manager);

	return 0;
}
