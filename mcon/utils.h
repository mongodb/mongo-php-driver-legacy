#ifndef __MCON_UTILS_H__
#define __MCON_UTILS_H__

#include "types.h"

char *mongo_server_create_hash(mongo_server_def *server_def);
int mongo_server_split_hash(char *hash, char **host, int *port, char **db, char **username, int *pid);
char *mongo_server_hash_to_server(char *hash);

#endif
