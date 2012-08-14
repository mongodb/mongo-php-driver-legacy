#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "parse.h"
#include "manager.h"

/* Forward declarations */
void static mongo_add_parsed_server_addr(mongo_con_manager *manager, mongo_servers *servers, char *host_start, char *host_end, char *port_start, char *port_end);
int static mongo_parse_options(mongo_con_manager *manager, mongo_servers *servers, char *options_string, char **error_message);

/* Parsing routine */
mongo_servers* mongo_parse_init(void)
{
	mongo_servers *servers;

	/* Create tmp server definitions */
	servers = malloc(sizeof(mongo_servers));
	memset(servers, 0, sizeof(mongo_servers));
	servers->count = 0;
	servers->repl_set_name = NULL;
	servers->con_type = MONGO_CON_TYPE_STANDALONE;

	return servers;
}

int mongo_parse_server_spec(mongo_con_manager *manager, mongo_servers *servers, char *spec, char **error_message)
{
	char          *pos; /* Pointer to current parsing position */
	char          *tmp_user = NULL, *tmp_pass = NULL; /* Stores parsed user/pw to be copied to each server struct */
	char          *host_start, *host_end, *port_start, *port_end, *db_start, *db_end;
	int            i;

	/* Initialisation */
	pos = spec;
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "Parsing %s", spec);

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
			int retval = -1;
			retval = mongo_parse_options(manager, servers, question + 1, error_message);
			if (retval > 0) {
				return 1;
			}
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

	free(tmp_user);
	free(tmp_pass);

	return 0;
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
 * Returns 0 if it worked, 1 if either name or value was missing, 2 if the option didn't exist, 3 on logic errors
 */
int static mongo_process_option(mongo_con_manager *manager, mongo_servers *servers, char *name, char *value, char *pos, char **error_message)
{
	char *tmp_name;
	char *tmp_value;
	int   retval = 0;

	if (!name || !value) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Got an empty option name or value");
		return 1;
	}

	tmp_name = strndup(name, value - name - 1);
	tmp_value = strndup(value, pos - value);

	retval = mongo_store_option(manager, servers, tmp_name, tmp_value, error_message);

	free(tmp_name);
	free(tmp_value);

	return retval;
}

/* Sets server options.
 * Returns 0 if it worked, 2 if the option didn't exist, 3 on logical errors.
 * On logical errors, the error_message will be populated with the reason.
 */
int mongo_store_option(mongo_con_manager *manager, mongo_servers *servers, char *option_name, char *option_value, char **error_message)
{
	int i;

	if (strcasecmp(option_name, "replicaSet") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'replicaSet': '%s'", option_value);

		if (servers->repl_set_name) {
			/* Free the already existing one */
			free(servers->repl_set_name);
			servers->repl_set_name = NULL; /* We reset it as not all options set a string as replset name */
		}

		if (option_value && *option_value) {
			servers->repl_set_name = strdup(option_value);
			servers->con_type = MONGO_CON_TYPE_REPLSET;
			mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: REPLSET");
		} else {
			/* Turn off replica set handling, which means either use a
			 * standalone server, or a "multi-set". Why you would do
			 * this? No idea. */
			if (servers->count == 1) {
				servers->con_type = MONGO_CON_TYPE_STANDALONE;
				mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: STANDALONE");
			} else {
				servers->con_type = MONGO_CON_TYPE_MULTIPLE;
				mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: MULTIPLE");
			}
		}
		return 0;
	}
	if (strcasecmp(option_name, "username") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'username': '%s'", option_value);
		for (i = 0; i < servers->count; i++) {
			if (servers->server[i]->username) {
				free(servers->server[i]->username);
			}
			servers->server[i]->username = strdup(option_value);
		}
		return 0;
	}
	if (strcasecmp(option_name, "password") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'password': '%s'", option_value);
		for (i = 0; i < servers->count; i++) {
			if (servers->server[i]->password) {
				free(servers->server[i]->password);
			}
			servers->server[i]->password = strdup(option_value);
		}
		return 0;
	}
	if (strcasecmp(option_name, "db") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'db': '%s'", option_value);
		for (i = 0; i < servers->count; i++) {
			if (servers->server[i]->db) {
				free(servers->server[i]->db);
			}
			servers->server[i]->db = strdup(option_value);
		}
		return 0;
	}
	if (strcasecmp(option_name, "slaveOkay") == 0) {
		if (strcasecmp(option_value, "true") == 0 || *option_value == '1') {
			mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'slaveOkay': true");
			if (servers->read_pref.type != MONGO_RP_PRIMARY || servers->read_pref.tagset_count) {
				/* the server already has read preferences configured, but we're still
				 * trying to set slave okay. The spec says that's an error */
				*error_message = strdup("You can not use both slaveOkay and read-preferences. Please switch to read-preferences.");
				return 3;
			} else {
				/* Old style option, that needs to be removed. For now, spec dictates
				 * it needs to be ReadPreference=SECONDARY_PREFERRED */
				servers->read_pref.type = MONGO_RP_SECONDARY_PREFERRED;
			}
			return 0;
		}

		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'slaveOkay': false");
		return 0;
	}

	if (strcasecmp(option_name, "timeout") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'timeout': %d", atoi(option_value));
		servers->connectTimeoutMS = atoi(option_value);
		return 0;
	}

	*error_message = malloc(256);
	snprintf(*error_message, 256, "- Found unknown option '%s' with value %s", option_name, option_value);
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found unknown option '%s' with value %s", option_name, option_value);
	return 2;
}


/* Returns 0 if all options were processed without errors.
 * On failure, returns 1 and populates error_message */
int static mongo_parse_options(mongo_con_manager *manager, mongo_servers *servers, char *options_string, char **error_message)
{
	int retval = 0;
	char *name_start, *value_start = NULL, *pos;

	name_start = pos = options_string;

	do {
		if (*pos == '=') {
			value_start = pos + 1;
		}
		if (*pos == ';' || *pos == '&') {
			retval = mongo_process_option(manager, servers, name_start, value_start, pos, error_message);
			/* An empty name/value isn't an error */
			if (retval > 1) {
				return 1;
			}
			name_start = pos + 1;
			value_start = NULL;
		}
		pos++;
	} while (*pos != '\0');
	retval = mongo_process_option(manager, servers, name_start, value_start, pos, error_message);

	/* An empty name/value isn't an error */
	return retval > 1 ? 1 : 0;
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
