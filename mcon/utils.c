#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

/* Creates a unique hash for a server def with some info from the server config,
 * but also with the PID to make sure forking works */
char *mongo_server_create_hash(mongo_server_def *server_def)
{
	char *tmp;
	int   size = 0;

	/* Host (string) and port (max 5 digits) + 2 separators */
	size += strlen(server_def->host) + 1 + 5 + 1;

	/* Database and username */
	if (server_def->db && server_def->username) {
		size += strlen(server_def->db) + 1 + strlen(server_def->username) + 1;
	}

	/* PID (assume max size, a signed 32bit int) */
	size += 10;

	/* Allocate and fill */
	tmp = malloc(size);
	sprintf(tmp, "%s:%d;", server_def->host, server_def->port);
	if (server_def->db && server_def->username) {
		sprintf(tmp + strlen(tmp), "%s/%s;", server_def->db, server_def->username);
	} else {
		sprintf(tmp + strlen(tmp), "X;");
	}
	sprintf(tmp + strlen(tmp), "%d", getpid());

	return tmp;
}
