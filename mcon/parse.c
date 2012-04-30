#include "parse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Forward declarations */
void static mongo_add_parsed_server_addr(mongo_servers *servers, char *host_start, char *host_end, char *port_start, char *port_end);

/* Parsing routine */
mongo_servers* mongo_parse_server_spec(char *spec)
{
	mongo_servers *servers;
	char          *pos; /* Pointer to current parsing position */
	char          *tmp_user = NULL, *tmp_pass = NULL; /* Stores parsed user/pw to be copied to each server struct */
	char          *host_start, *host_end, *port_start, *port_end;
	int            i;

	/* Initialisation */
	pos = spec;

	/* Create tmp server definitions */
	servers = malloc(sizeof(mongo_servers));
	servers->count = 0;

	if (strstr(spec, "mongodb://") == spec) {
		char *at, *colon;

		/* mongodb://user:pass@host:port,host:port
		 *           ^                             */
		pos += 10;

		/* mongodb://user:pass@host:port,host:port
		 *                    ^                    */
		at = strchr(pos, '@');

		/* mongodb://user:pass@host:port,host:port
		 *               ^                         */
		colon = strchr(pos, ':');

		/* check for username:password */
		if (at && colon && at - colon > 0) {
			tmp_user = strndup(pos, colon - pos);
			tmp_pass = strndup(colon + 1, at - (colon + 1));

			/* move current
			 * mongodb://user:pass@host:port,host:port
			 *                     ^                   */
			pos = at + 1;
		}
	}

	host_start = pos;
	host_end   = NULL;
	port_start = NULL;
	port_end   = NULL;

	/* Now we parse the host:port parts up to the / which starts the dbname */
	do {
		if (*pos == ':') {
			host_end = pos;
			port_start = pos + 1;
		}
		if (*pos == ',') {
			if (!host_end) {
				host_end = pos;
			} else {
				port_end = pos;
			}

			mongo_add_parsed_server_addr(servers, host_start, host_end, port_start, port_end);

			host_start = pos + 1;
			host_end = port_start = port_end = NULL;
		}
		if (*pos == '/') {
			if (!host_end) {
				host_end = pos;
			} else {
				port_end = pos;
			}
			break;
		}
		pos++;
	} while (*pos != '\0');

	/* We are now either at the end of the string, or at / where the dbname starts.
	 * We still have to add the last parser host/port combination though: */
	mongo_add_parsed_server_addr(servers, host_start, host_end, port_start, port_end);

	/* Update all servers with user, password and dbname */
	for (i = 0; i < servers->count; i++) {
		servers->server[i]->username = tmp_user ? strdup(tmp_user) : NULL;
		servers->server[i]->password = tmp_pass ? strdup(tmp_pass) : NULL;
		if (*pos == '/') {
			servers->server[i]->db = strdup(pos + 1);
		} else {
			servers->server[i]->db = NULL;
		}
	}

	free(tmp_user);
	free(tmp_pass);

	return servers;
}

/* Helpers */
void static mongo_add_parsed_server_addr(mongo_servers *servers, char *host_start, char *host_end, char *port_start, char *port_end)
{
	mongo_server_def *tmp;

	tmp = malloc(sizeof(mongo_server_def));
	tmp->username = tmp->password = tmp->db = NULL;
	tmp->port = 27017;

	tmp->host = strndup(host_start, host_end - host_start);
	if (port_start) {
		tmp->port = atoi(port_start);
	}
	servers->server[servers->count] = tmp;
	servers->count++;
}

void static mongo_server_def_dump(mongo_server_def *server_def)
{
	printf("host: %s; port: %d; username: %s, password: %s\n",
		server_def->host, server_def->port, server_def->username, server_def->password);
}

void mongo_servers_dump(mongo_servers *servers)
{
	int i;

	for (i = 0; i < servers->count; i++) {
		mongo_server_def_dump(servers->server[i]);
	}
	printf("\n");
}

/* Cleanup */
void static mongo_server_def_dtor(mongo_server_def *server_def)
{
	if (server_def->host) {
		free(server_def->host);
	}
	if (server_def->db) {
		free(server_def->db);
	}
	if (server_def->username) {
		free(server_def->username);
	}
	if (server_def->password) {
		free(server_def->password);
	}
	free(server_def);
}

void mongo_servers_dtor(mongo_servers *servers)
{
	int i;

	for (i = 0; i < servers->count; i++) {
		mongo_server_def_dtor(servers->server[i]);
	}
	free(servers);
}
