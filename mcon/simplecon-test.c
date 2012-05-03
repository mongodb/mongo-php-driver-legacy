#include "parse.h"
#include "manager.h"

int main(void)
{
	mongo_servers *servers;
	mongo_con_manager *manager;
	mongo_connection *con;

	manager = mongo_init();

	servers = mongo_parse_server_spec("mongodb://10.0.1.1:27000");
	mongo_servers_dump(servers);

	con = mongo_get_connection(manager, servers);

	mongo_servers_dtor(servers);
}
