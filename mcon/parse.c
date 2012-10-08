#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "parse.h"
#include "manager.h"
#include "read_preference.h"

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
	char          *tmp_user = NULL, *tmp_pass = NULL, *tmp_database = NULL; /* Stores parsed user/password/database to be copied to each server struct */
	char          *host_start, *host_end, *port_start, *port_end, *db_start, *db_end, *last_slash;
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
	last_slash = NULL;

	/* Now we parse the host part - there are two cases:
	 * 1: mongodb://user:pass@host:port,host:port/database?opt=1 -- TCP/IP
	 *                        ^
	 * 2: mongodb://user:pass@/tmp/mongo.sock/database?opt=1 -- Unix Domain sockets
	 *                        ^                                                     */
	if (*pos != '/') {
		/* TCP/IP:
		 * mongodb://user:pass@host:port,host:port/database?opt=1 -- TCP/IP
		 *                     ^                                            */
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
	} else if (*pos == '/') {
		host_start = pos;
		port_start = "0";
		port_end   = NULL;

		/* Unix Domain Socket
		 * mongodb://user:pass@/tmp/mongo.sock
		 * mongodb://user:pass@/tmp/mongo.sock/?opt=1
		 * mongodb://user:pass@/tmp/mongo.sock/database?opt=1
		 */
		last_slash = strrchr(pos, '/');

		/* The last component of the path *could* be a database name.
		 * The rule is; if the last component has a dot, we use the full string since "host_start" as host */
		if (strchr(last_slash, '.')) {
			host_end = host_start + strlen(host_start);
		} else {
			host_end = last_slash;
		}
		pos = host_end;
		mongo_add_parsed_server_addr(manager, servers, host_start, host_end, port_start, port_end);
	}

	/* Set the default connection type, we might change this if we encounter
	 * the replicaSet option later */
	if (servers->count == 1) {
		servers->con_type = MONGO_CON_TYPE_STANDALONE;
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: STANDALONE");
	} else {
		servers->con_type = MONGO_CON_TYPE_MULTIPLE;
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Connection type: MULTIPLE");
	}

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
				free(tmp_user);
				free(tmp_pass);
				free(tmp_database);

				return 1;
			}
		}
	}

	/* Handling database name */
	if (db_start) {
		tmp_database = strndup(db_start, db_end - db_start);
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found database name '%s'", tmp_database);
	} else if (tmp_user && tmp_pass) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- No database name found for an authenticated connection. Using 'admin' as default database");
		tmp_database = strdup("admin");
	}

	/* Update all servers with user, password and dbname */
	for (i = 0; i < servers->count; i++) {
		servers->server[i]->username = tmp_user ? strdup(tmp_user) : NULL;
		servers->server[i]->password = tmp_pass ? strdup(tmp_pass) : NULL;
		servers->server[i]->db       = tmp_database ? strdup(tmp_database) : NULL;
	}

	free(tmp_user);
	free(tmp_pass);
	free(tmp_database);

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

/* Option parser helpers */
static int parse_read_preference_tags(mongo_con_manager *manager, mongo_servers *servers, char *value, char **error_message)
{
	mongo_read_preference_tagset *tmp_ts = calloc(1, sizeof(mongo_read_preference_tagset));

	/* format = dc:ny,rack:1 - empty is allowed! */
	if (strlen(value) == 0) {
		mongo_read_preference_add_tagset(&servers->read_pref, tmp_ts);
	} else {
		char *start, *end, *colon, *tmp_name, *tmp_value;

		start = value;

		while (1) {
			end = strchr(start, ',');
			colon = strchr(start, ':');
			if (!colon) {
				*error_message = malloc(256 + strlen(start));
				snprintf(*error_message, 256 + strlen(start), "Error while trying to parse tags: No separator for '%s'", start);
				mongo_read_preference_tagset_dtor(tmp_ts);
				return 3;
			}
			tmp_name = strndup(start, colon - start);
			if (end) {
				tmp_value = strndup(colon + 1, end - colon - 1);
				start = end + 1;
				mongo_read_preference_add_tag(tmp_ts, tmp_name, tmp_value);
				free(tmp_value);
				free(tmp_name);
			} else {
				mongo_read_preference_add_tag(tmp_ts, tmp_name, colon + 1);
				free(tmp_name);
				break;
			}
		}
		mongo_read_preference_add_tagset(&servers->read_pref, tmp_ts);
	}
	return 0;
}

/* Sets server options.
 * Returns 0 if it worked, 2 if the option didn't exist, 3 on logical errors.
 * On logical errors, the error_message will be populated with the reason.
 */
int mongo_store_option(mongo_con_manager *manager, mongo_servers *servers, char *option_name, char *option_value, char **error_message)
{
	int i;

	if (strcasecmp(option_name, "replicaSet") == 0) {
		if (servers->repl_set_name) {
			/* Free the already existing one */
			free(servers->repl_set_name);
			servers->repl_set_name = NULL; /* We reset it as not all options set a string as replset name */
		}

		if (option_value && *option_value) {
			/* We explicitly check for the stringified version of "true" here,
			 * as "true" has a special meaning. It does not mean that the
			 * replicaSet name is "1". */
			if (strcmp(option_value, "1") != 0) {
				servers->repl_set_name = strdup(option_value);
				mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'replicaSet': '%s'", option_value);
			} else {
				mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'replicaSet': true");
			}
			servers->con_type = MONGO_CON_TYPE_REPLSET;
			mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Switching connection type: REPLSET");
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
	if (strcasecmp(option_name, "readPreference") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'readPreference': '%s'", option_value);
		if (strcasecmp(option_value, "primary") == 0) {
			servers->read_pref.type = MONGO_RP_PRIMARY;
		} else if (strcasecmp(option_value, "primaryPreferred") == 0) {
			servers->read_pref.type = MONGO_RP_PRIMARY_PREFERRED;
		} else if (strcasecmp(option_value, "secondary") == 0) {
			servers->read_pref.type = MONGO_RP_SECONDARY;
		} else if (strcasecmp(option_value, "secondaryPreferred") == 0) {
			servers->read_pref.type = MONGO_RP_SECONDARY_PREFERRED;
		} else if (strcasecmp(option_value, "nearest") == 0) {
			servers->read_pref.type = MONGO_RP_NEAREST;
		} else {
			int len = strlen(option_value) + sizeof("The readPreference value '' is not supported.");

			*error_message = malloc(len + 1);
			snprintf(*error_message, len, "The readPreference value '%s' is not supported.", option_value);
			return 3;
		}
		return 0;
	}
	if (strcasecmp(option_name, "readPreferenceTags") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'readPreferenceTags': '%s'", option_value);
		return parse_read_preference_tags(manager, servers, option_value, error_message);
	}

	if (strcasecmp(option_name, "timeout") == 0) {
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found option 'timeout': %d", atoi(option_value));
		servers->connectTimeoutMS = atoi(option_value);
		return 0;
	}

	*error_message = malloc(256);
	snprintf(*error_message, 256, "- Found unknown connection string option '%s' with value '%s'", option_name, option_value);
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- Found unknown connection string option '%s' with value '%s'", option_name, option_value);
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
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- readPreference: %s", mongo_read_preference_type_to_name(servers->read_pref.type));
	for (i = 0; i < servers->read_pref.tagset_count; i++) {
		char *tmp = mongo_read_preference_squash_tagset(servers->read_pref.tagsets[i]);
		mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "- tagset: %s", tmp);
		free(tmp);
	}
	mongo_manager_log(manager, MLOG_PARSE, MLOG_INFO, "\n");
}

/* Cloning */
static void mongo_server_def_copy(mongo_server_def *to, mongo_server_def *from, int flags)
{
	to->host = to->db = to->username = to->password = NULL;
	if (from->host) {
		to->host = strdup(from->host);
	}
	to->port = from->port;

	if (flags & MONGO_SERVER_COPY_CREDENTIALS) {
		if (from->db) {
			to->db = strdup(from->db);
		}
		if (from->username) {
			to->username = strdup(from->username);
		}
		if (from->password) {
			to->password = strdup(from->password);
		}
	}
}

void mongo_servers_copy(mongo_servers *to, mongo_servers *from, int flags)
{
	int i;

	to->count = from->count;
	for (i = 0; i < from->count; i++) {
		to->server[i] = malloc(sizeof(mongo_server_def));
		mongo_server_def_copy(to->server[i], from->server[i], flags);
	}

	to->con_type = from->con_type;
	to->repl_set_name = NULL;
	to->connectTimeoutMS = from->connectTimeoutMS;

	if (from->repl_set_name) {
		to->repl_set_name = strdup(from->repl_set_name);
	}
	mongo_read_preference_copy(&from->read_pref, &to->read_pref);
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
	for (i = 0; i < servers->read_pref.tagset_count; i++) {
		mongo_read_preference_tagset_dtor(servers->read_pref.tagsets[i]);
	}
	if (servers->read_pref.tagsets) {
		free(servers->read_pref.tagsets);
	}
	free(servers);
}
