#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "parse.h"
#include "manager.h"

/* Forward declarations */
void static mongo_add_parsed_server_addr(mongo_con_manager *manager, mongo_servers *servers, char *host_start, char *host_end, char *port_start, char *port_end);
void static mongo_parse_options(mongo_con_manager *manager, mongo_servers *servers, char *options_string);

/* Parsing routine */
mongo_servers* mongo_parse_server_spec(mongo_con_manager *manager, char *spec)
{
	mongo_servers *servers;
	char          *pos; /* Pointer to current parsing position */
	char          *tmp_user = NULL, *tmp_pass = NULL; /* Stores parsed user/pw to be copied to each server struct */
	char          *host_start, *host_end, *port_start, *port_end, *db_start, *db_end;
	int            i;

	/* Initialisation */
	pos = spec;
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "Parsing %s", spec);

	/* Create tmp server definitions */
	servers = malloc(sizeof(mongo_servers));
	memset(servers, 0, sizeof(mongo_servers));
	servers->count = 0;
	servers->repl_set_name = NULL;

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
			mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found user '%s' and a password", tmp_user);
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

			mongo_add_parsed_server_addr(manager, servers, host_start, host_end, port_start, port_end);

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
	mongo_add_parsed_server_addr(manager, servers, host_start, host_end, port_start, port_end);

	/* Check for dbname
	 * mongodb://user:pass@host:port,host:port/dbname?foo=bar
	 *                                        ^ */
	db_start = NULL;
	db_end = spec + strlen(spec);
	if (*pos == '/') {
		char *question;

		question = strchr(pos, '?');
		if (question) {
			if (pos + 1 == question) {
				db_start = NULL;
			} else {
				db_start = pos + 1;
				db_end = question;
			}
		} else {
			db_start = pos + 1;
			db_end = spec + strlen(spec);
		}

		/* Check for options
		 * mongodb://user:pass@host:port,host:port/dbname?foo=bar
		 *                                               ^ */
		if (question) {
			mongo_parse_options(manager, servers, question + 1);
		}
	}
	if (db_start) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found database name '%s'", db_start);
	}

	/* Update all servers with user, password and dbname */
	for (i = 0; i < servers->count; i++) {
		servers->server[i]->username = tmp_user ? strdup(tmp_user) : NULL;
		servers->server[i]->password = tmp_pass ? strdup(tmp_pass) : NULL;
		servers->server[i]->db       = db_start ? strndup(db_start, db_end-db_start) : NULL;
	}

	/* Update connection type */
	if (servers->repl_set_name) {
		servers->con_type = MONGO_CON_TYPE_REPLSET;
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: REPLSET");
	} else {
		if (servers->count > 1) {
			servers->con_type = MONGO_CON_TYPE_MULTIPLE;
			mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: MULTIPLE");
		} else {
			servers->con_type = MONGO_CON_TYPE_STANDALONE;
			mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: STANDALONE");
		}
	}

	free(tmp_user);
	free(tmp_pass);

	return servers;
}

/* Helpers */
void static mongo_add_parsed_server_addr(mongo_con_manager *manager, mongo_servers *servers, char *host_start, char *host_end, char *port_start, char *port_end)
{
	mongo_server_def *tmp;

	tmp = malloc(sizeof(mongo_server_def));
	memset(tmp, 0, sizeof(mongo_server_def));
	tmp->username = tmp->password = tmp->db = NULL;
	tmp->port = 27017;

	tmp->host = strndup(host_start, host_end - host_start);
	if (port_start) {
		tmp->port = atoi(port_start);
	}
	servers->server[servers->count] = tmp;
	servers->count++;
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found node: %s:%d", tmp->host, tmp->port);
}

/* Processes a single option/value pair.
 * Returns 0 if it worked, 1 if either name or value was missing and 2 if the option didn't exist
 */
int static mongo_process_option(mongo_con_manager *manager, mongo_servers *servers, char *name, char *value, char *pos)
{
	char *tmp_name;
	char *tmp_value;
	int   retval = 0;

	if (!name || !value) {
		return 1;
	}

	tmp_name = strndup(name, value - name - 1);
	tmp_value = strndup(value, pos - value);

	if (strcmp(tmp_name, "replicaSet") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'replicaSet': %s", tmp_value);
		servers->repl_set_name = strdup(tmp_value);
	} else {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found unknown option '%s' with value %s", tmp_name, tmp_value);
		retval = 2;
	}

	free(tmp_name);
	free(tmp_value);

	return retval;
}

void static mongo_parse_options(mongo_con_manager *manager, mongo_servers *servers, char *options_string)
{
	char *name_start, *value_start = NULL, *pos;

	name_start = pos = options_string;

	do {
		if (*pos == '=') {
			value_start = pos + 1;
		}
		if (*pos == ';' || *pos == '&') {
			mongo_process_option(manager, servers, name_start, value_start, pos);
			name_start = pos + 1;
			value_start = NULL;
		}
		pos++;
	} while (*pos != '\0');
	mongo_process_option(manager, servers, name_start, value_start, pos);
}

void static mongo_server_def_dump(mongo_con_manager *manager, mongo_server_def *server_def)
{
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO,
		"- host: %s; port: %d; username: %s, password: %s, database: %s",
		server_def->host, server_def->port, server_def->username, server_def->password, server_def->db);
}

void mongo_servers_dump(mongo_con_manager *manager, mongo_servers *servers)
{
	int i;

	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "Seeds:");
	for (i = 0; i < servers->count; i++) {
		mongo_server_def_dump(manager, servers->server[i]);
	}
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "");

	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "Options:");
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- repl_set_name: %s", servers->repl_set_name);
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "\n");
}

/* Cleanup */
void mongo_server_def_dtor(mongo_server_def *server_def)
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
	if (servers->repl_set_name) {
		free(servers->repl_set_name);
	}
	free(servers);
}
