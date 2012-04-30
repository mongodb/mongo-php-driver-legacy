#include "parse.h"

void parse_test(char *spec)
{
	mongo_servers *servers;

	servers = mongo_parse_server_spec(spec);
	mongo_servers_dump(servers);
	mongo_servers_dtor(servers);
}

int main(void)
{
	parse_test("host1:123");
	parse_test("host1:123,host2:123");
	parse_test("mongodb://host1:123,host2:123");
	parse_test("mongodb://derick:test@host1:123");
	parse_test("mongodb://derick:test@host1:123,host2:123");
	parse_test("mongodb://derick:test@host1:123,host2:123/database");
	parse_test("mongodb://derick:test@host1:123,host2/database");
	parse_test("mongodb://derick:test@host1,host2:123/database");
}
