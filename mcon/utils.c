/**
 *  Copyright 2009-2014 MongoDB, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "types.h"
#include "utils.h"
#include "contrib/md5.h"
#include "contrib/strndup.h"

/* Creates a hash out of the password so that we can store it in the connection hash.
 * The format of this hash is md5(PID,PASSWORD,USERNAME). */
char *mongo_server_create_hashed_password(char *username, char *password)
{
	int salt_length, length;
	char *hash, *salt;

	/* First we create a hash for the password to store (md5(pid,password,username)) */
	salt_length = strlen(password) + strlen(username) + 10 + 1; /* 10 for the 32bit int at max size */
	salt = malloc(salt_length);
	length = snprintf(salt, salt_length, "%d%s%s", getpid(), password, username);
	hash = mongo_util_md5_hex(salt, length);
	free(salt);

	return hash;
}

/* Hash format is:
 * - HOST:PORT;-;.;PID (with the - being the replica set name and the . a placeholder for credentials)
 * or:
 * - HOST:PORT;REPLSETNAME;DB/USERNAME/md5(PID,PASSWORD,USERNAME);PID */

/* Creates a unique hash for a server def with some info from the server config,
 * but also with the PID to make sure forking works */
char *mongo_server_create_hash(mongo_server_def *server_def)
{
	char *tmp, *hash;
	int   size = 0;

	/* Host (string) and port (max 5 digits) + 2 separators */
	size += strlen(server_def->host) + 1 + 5 + 1;

	/* Replica set name */
	if (server_def->repl_set_name) {
		size += strlen(server_def->repl_set_name) + 1;
	} else {
		size += 2;
	}

	/* Database, username and hashed password */
	if (server_def->db && server_def->username && server_def->password) {
		hash = mongo_server_create_hashed_password(server_def->username, server_def->password);
		size += strlen(server_def->db) + 1 + strlen(server_def->username) + 1 + strlen(hash) + 1;
	} else {
		size += 2;
	}

	/* PID (assume max size, a signed 32bit int) */
	size += 10;

	/* Add one for the \0 at the end */
	size += 1;

	/* Allocate and fill */
	tmp = malloc(size);
	sprintf(tmp, "%s:%d;", server_def->host, server_def->port);
	if (server_def->repl_set_name) {
		sprintf(tmp + strlen(tmp), "%s;", server_def->repl_set_name);
	} else {
		sprintf(tmp + strlen(tmp), "-;");
	}
	if (server_def->db && server_def->username && server_def->password) {
		sprintf(tmp + strlen(tmp), "%s/%s/%s;", server_def->db, server_def->username, hash);
		free(hash);
	} else {
		sprintf(tmp + strlen(tmp), ".;");
	}
	sprintf(tmp + strlen(tmp), "%d", getpid());

	return tmp;
}

/* Split a hash back into its constituent parts */
int mongo_server_split_hash(char *hash, char **host, int *port, char **repl_set_name, char **database, char **username, char **auth_hash, int *pid)
{
	char *ptr, *pid_semi, *username_slash;

	ptr = hash;

	/* Find the host */
	ptr = strchr(ptr, ':');
	if (host) {
		*host = mcon_strndup(hash, ptr - hash);
	}

	/* Find the port */
	if (port) {
		*port = atoi(ptr + 1);
	}

	/* Find the replica set name */
	ptr = strchr(ptr, ';') + 1;
	if (ptr[0] != '-') {
		if (repl_set_name) {
			*repl_set_name = mcon_strndup(ptr, strchr(ptr, ';') - ptr);
		}
	} else {
		if (repl_set_name) {
			*repl_set_name = NULL;
		}
	}

	/* Find the database and username */
	ptr = strchr(ptr, ';') + 1;
	if (ptr[0] != '.') {
		if (database) {
			*database = mcon_strndup(ptr, strchr(ptr, '/') - ptr);
		}
		username_slash = strchr(ptr, '/');
		if (username) {
			*username = mcon_strndup(username_slash + 1, strchr(username_slash + 1, '/') - username_slash - 1);
		}
		pid_semi = strchr(ptr, ';');
		if (auth_hash) {
			*auth_hash = mcon_strndup(strchr(username_slash + 1, '/') + 1, pid_semi - strchr(username_slash + 1, '/') - 1);
		}
	} else {
		if (database) {
			*database = NULL;
		}
		if (username) {
			*username = NULL;
		}
		if (auth_hash) {
			*auth_hash = NULL;
		}
		pid_semi = strchr(ptr, ';');
	}

	/* Find the PID */
	if (pid) {
		*pid = atoi(pid_semi + 1);
	}

	return 0;
}

/* Returns just the host and port from the hash */
char *mongo_server_hash_to_server(char *hash)
{
	char *ptr, *tmp;

	ptr = strchr(hash, ';');
	tmp = mcon_strndup(hash, ptr - hash);
	return tmp;
}

/* Returns just the PID from the hash */
int mongo_server_hash_to_pid(char *hash)
{
	char *ptr;

	ptr = strrchr(hash, ';');
	return atoi(ptr+1);
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
