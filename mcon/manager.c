#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

mongo_con_manager *mongo_init(void)
{
	mongo_con_manager *tmp;

	tmp = malloc(sizeof(mongo_con_manager));
	memset(tmp, 0, sizeof(mongo_con_manager));

	return tmp;
}

void mongo_deinit(mongo_con_manager *manager)
{
	free(manager);
}

mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash)
{
}

mongo_connection *mongo_manager_connection_register(mongo_con_manager *manager, char *hash, mongo_connection *con)
{
}

