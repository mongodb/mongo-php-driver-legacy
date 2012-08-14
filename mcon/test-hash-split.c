#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
	char *hash1, *hash2;
	char *host, *db, *username;
	int port, pid;

	mongo_server_def def1 = { "whisky", 13000, NULL, NULL, NULL };
	mongo_server_def def2 = { "whisky", 13000, "phpunit", "derick", "not!" };

	hash1 = mongo_server_create_hash(&def1);
	mongo_server_split_hash(hash1, &host, &port, &db, &username, &pid);
	printf("HASH: %s; host: %s, port: %ld, db: %s, username: %s, pid: %d\n",
		hash1, host, port, db, username, pid);

	hash2 = mongo_server_create_hash(&def2);
	mongo_server_split_hash(hash2, &host, &port, &db, &username, &pid);
	printf("HASH: %s; host: %s, port: %ld, db: %s, username: %s, pid: %d\n",
		hash2, host, port, db, username, pid);
}
