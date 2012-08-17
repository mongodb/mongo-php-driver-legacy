#include <string.h>
#include "types.h"
#include "utils.h"
#include "parse.h"
#include "manager.h"
#include "connections.h"
#include "io.h"
#include "str.h"
#include "bson_helpers.h"
#include "mini_bson.h"

#ifdef WIN32
#include <winsock2.h>
#  ifndef int64_t
     typedef __int64 int64_t;
#  endif
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/un.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#define INT_32  4
#define FLAGS   0

#define MONGO_REPLY_FLAG_QUERY_FAILURE 0x02

/* Helper functions */
int mongo_connection_get_reqid(mongo_connection *con)
{
	con->last_reqid++;
	return con->last_reqid;
}

static int mongo_util_connect__sockaddr(struct sockaddr *sa, int family, char *host, int port, char **errmsg)
{
#ifndef WIN32
	if (family == AF_UNIX) {
		struct sockaddr_un* su = (struct sockaddr_un*)(sa);
		su->sun_family = AF_UNIX;
		strncpy(su->sun_path, host, sizeof(su->sun_path));
	} else {
#endif
		struct hostent *hostinfo;
		struct sockaddr_in* si = (struct sockaddr_in*)(sa);

		si->sin_family = AF_INET;
		si->sin_port = htons(port);
		hostinfo = (struct hostent*)gethostbyname(host);

		if (hostinfo == NULL) {
			*errmsg = malloc(256);
			snprintf(*errmsg, 256, "Couldn't get host info for %s", host);
			return 0;
		}

#ifndef WIN32
		si->sin_addr = *((struct in_addr*)hostinfo->h_addr);
	}
#else
	si->sin_addr.s_addr = ((struct in_addr*)(hostinfo->h_addr))->s_addr;
#endif

	return 1;
}

/* This function does the actual connecting */
int mongo_connection_connect(char *host, int port, int timeout, char **error_message)
{
	struct sockaddr*   sa;
	struct sockaddr_in si;
	socklen_t          sn;
	int                family;
	struct timeval     tval;
	int                connected;
	int                status;
	int                tmp_socket;

	*error_message = NULL;

#ifdef WIN32
	WORD       version;
	WSADATA    wsaData;
	int        size, error;
	u_long     no = 0;
	const char yes = 1;
#else
	struct sockaddr_un su;
	uint               size;
	int                yes = 1;
#endif

#ifdef WIN32
	family = AF_INET;
	sa = (struct sockaddr*)(&si);
	sn = sizeof(si);

	version = MAKEWORD(2,2);
	error = WSAStartup(version, &wsaData);

	if (error != 0) {
		return -1;
	}

	/* create socket */
	tmp_socket = socket(family, SOCK_STREAM, 0);
	if (tmp_socket == INVALID_SOCKET) {
		*error_message = strdup(strerror(errno));
		return -1;
	}

#else
	/* domain socket */
	if (port == 0) {
		family = AF_UNIX;
		sa = (struct sockaddr*)(&su);
		sn = sizeof(su);
	} else {
		family = AF_INET;
		sa = (struct sockaddr*)(&si);
		sn = sizeof(si);
	}

	/* create socket */
	if ((tmp_socket = socket(family, SOCK_STREAM, 0)) == -1) {
		*error_message = strdup(strerror(errno));
		return -1;
	}
#endif

	/* TODO: Move this to within the loop & use real timeout setting */
	/* connection timeout: set in ms (current default 1000 secs) */
	tval.tv_sec = timeout <= 0 ? 1000 : timeout / 1000;
	tval.tv_usec = timeout <= 0 ? 0 : (timeout % 1000) * 1000;

	/* get addresses */
	if (mongo_util_connect__sockaddr(sa, family, host, port, error_message) == 0) {
		goto error;
	}

	setsockopt(tmp_socket, SOL_SOCKET, SO_KEEPALIVE, &yes, INT_32);
	setsockopt(tmp_socket, IPPROTO_TCP, TCP_NODELAY, &yes, INT_32);

#ifdef WIN32
	ioctlsocket(tmp_socket, FIONBIO, (u_long*)&yes);
#else
	fcntl(tmp_socket, F_SETFL, FLAGS|O_NONBLOCK);
#endif

	/* connect */
	status = connect(tmp_socket, sa, sn);
	if (status < 0) {
#ifdef WIN32
		errno = WSAGetLastError();
		if (errno != WSAEINPROGRESS && errno != WSAEWOULDBLOCK) {
#else
		if (errno != EINPROGRESS) {
#endif
			*error_message = strdup(strerror(errno));
			goto error;
		}

		while (1) {
			fd_set rset, wset, eset;

			FD_ZERO(&rset);
			FD_SET(tmp_socket, &rset);
			FD_ZERO(&wset);
			FD_SET(tmp_socket, &wset);
			FD_ZERO(&eset);
			FD_SET(tmp_socket, &eset);

			if (select(tmp_socket+1, &rset, &wset, &eset, &tval) == 0) {
				*error_message = malloc(256);
				snprintf(*error_message, 256, "Timed out after %d ms", timeout);
				goto error;
			}

			/* if our descriptor has an error */
			if (FD_ISSET(tmp_socket, &eset)) {
				*error_message = strdup(strerror(errno));
				goto error;
			}

			/* if our descriptor is ready break out */
			if (FD_ISSET(tmp_socket, &wset) || FD_ISSET(tmp_socket, &rset)) {
				break;
			}
		}

		size = sn;

		connected = getpeername(tmp_socket, sa, &size);
		if (connected == -1) {
			*error_message = strdup(strerror(errno));
			goto error;
		}
	}

	/* reset flags */
#ifdef WIN32
	ioctlsocket(tmp_socket, FIONBIO, &no);
#else
	fcntl(tmp_socket, F_SETFL, FLAGS);
#endif
	return tmp_socket;

error:
#ifdef WIN32
	shutdown((tmp_socket), 2);
	closesocket(tmp_socket);
	WSACleanup();
#else
	shutdown((tmp_socket), 2);
	close(tmp_socket);
#endif
	return -1;
}

mongo_connection *mongo_connection_create(mongo_con_manager *manager, mongo_server_def *server_def, char **error_message)
{
	mongo_connection *tmp;

	/* Init struct */
	tmp = malloc(sizeof(mongo_connection));
	memset(tmp, 0, sizeof(mongo_connection));
	tmp->last_reqid = rand();

	/* Connect */
	mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "connection_create: creating new connection for %s:%d", server_def->host, server_def->port);
	tmp->socket = mongo_connection_connect(server_def->host, server_def->port, 1000, error_message);
	if (tmp->socket == -1) {
		mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "connection_create: error while creating connection for %s:%d: %s", server_def->host, server_def->port, *error_message);
		free(tmp);
		return NULL;
	}

	/* We call get_server_flags to the maxBsonObjectSize data */
	mongo_connection_get_server_flags(manager, tmp, (char**) &error_message);

	return tmp;
}

void mongo_connection_destroy(mongo_con_manager *manager, mongo_connection *con)
{
	int current_pid, connection_pid;
	int i;

	current_pid = getpid();
	connection_pid = mongo_server_hash_to_pid(con->hash);

	/* Only close the connection if it matches the current PID */
	if (current_pid != connection_pid) {
		mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "mongo_connection_destroy: The process pid (%d) for %s doesn't match the connection pid (%d).", current_pid, con->hash, connection_pid);
	} else {
		mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "mongo_connection_destroy: Closing socket for %s.", con->hash);

#ifdef WIN32
		shutdown(con->socket, SD_BOTH);
		closesocket(con->socket);
		WSACleanup();
#else
		shutdown(con->socket, SHUT_RDWR);
		close(con->socket);
#endif
		for (i = 0; i < con->tag_count; i++) {
			free(con->tags[i]);
		}
		free(con->tags);
		free(con->hash);
		free(con);
	}
}

#define MONGO_REPLY_HEADER_SIZE 36

static int mongo_connect_send_packet(mongo_con_manager *manager, mongo_connection *con, mcon_str *packet, char **data_buffer, char **error_message)
{
	int            read;
	uint32_t       data_size;
	char           reply_buffer[MONGO_REPLY_HEADER_SIZE];
	uint32_t       flags; /* To check for query reply status */

	/* Send and wait for reply */
	mongo_io_send(con->socket, packet->d, packet->l, error_message);
	mcon_str_ptr_dtor(packet);
	read = mongo_io_recv_header(con->socket, reply_buffer, MONGO_REPLY_HEADER_SIZE, error_message);

	mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "send_packet: read from header: %d", read);
	if (read < MONGO_REPLY_HEADER_SIZE) {
		return 0;
	}

	/* Check for a query error */
	flags = MONGO_32(*(int*)(reply_buffer + sizeof(int32_t) * 4));
	if (flags & MONGO_REPLY_FLAG_QUERY_FAILURE) {
		return 0;
	}

	/* Read the rest of the data */
	data_size = MONGO_32(*(int*)(reply_buffer)) - MONGO_REPLY_HEADER_SIZE;
	mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "send_packet: data_size: %d", data_size);

	/* TODO: Check size limits */
	*data_buffer = malloc(data_size + 1);
	if (!mongo_io_recv_data(con->socket, *data_buffer, data_size, error_message)) {
		free(data_buffer);
		return 0;
	}

	return 1;
}

/**
 * Sends a ping command to the server and stores the result.
 *
 * Returns 1 when it worked, and 0 when an error was encountered.
 */
int mongo_connection_ping(mongo_con_manager *manager, mongo_connection *con)
{
	mcon_str      *packet;
	char          *error_message = NULL;
	struct timeval start, end;
	char          *data_buffer;

	mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "is_ping: start");
	packet = bson_create_ping_packet(con);

	gettimeofday(&start, NULL);
	if (!mongo_connect_send_packet(manager, con, packet, &data_buffer, (char **) &error_message)) {
		return 0;
	}
	gettimeofday(&end, NULL);
	free(data_buffer);

	con->last_ping = end.tv_sec;
	con->ping_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
	if (con->ping_ms < 0) { /* some clocks do weird stuff */
		con->ping_ms = 0;
	}

	mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "is_ping: last pinged at %ld; time: %dms", con->last_ping, con->ping_ms);

	return 1;
}

/**
 * Sends an replSetGetStatus command to the server and returns an array of new
 * connectable nodes
 *
 * Returns:
 * 0: when an error occurred
 * 1: when is master was run and worked
 * 2: when is master wasn't run due to the time-out limit
 * 3: when it all worked, but we need to remove the seed host (due to its name
 *    not being what the server thought it is) - in that case, the server in
 *    the last argument is changed
 */
int mongo_connection_rs_status(mongo_con_manager *manager, mongo_connection *con, char **repl_set_name, int *nr_hosts, char ***found_hosts, char **error_message, mongo_server_def *server)
{
	mcon_str      *packet;
	char          *data_buffer;
	char          *set = NULL;      /* For replicaset in return */
	char          *hosts, *ptr, *member_ptr;
	char          *we_think_we_are;
	struct timeval now;
	int            retval = 1;

	gettimeofday(&now, NULL);
	if (con->last_is_master + manager->is_master_interval > now.tv_sec) {
		mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "rs_status: skipping: last ran at %ld, now: %ld, time left: %ld", con->last_is_master, now.tv_sec, con->last_is_master + manager->is_master_interval - now.tv_sec);
		return 2;
	}

	mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "rs_status: start");
	packet = bson_create_rs_status_packet(con);

	if (!mongo_connect_send_packet(manager, con, packet, &data_buffer, error_message)) {
		return 0;
	}

	/* Find data fields */
	ptr = data_buffer + sizeof(int32_t); /* Skip the length */

	/* Do replica set name test */
	bson_find_field_as_string(ptr, "set", &set);
	if (!set) {
		*error_message = strdup("Not a replicaset member");
		free(data_buffer);
		return 0;
	} else if (*repl_set_name) {
		if (strcmp(set, *repl_set_name) != 0) {
			struct mcon_str *tmp;

			mcon_str_ptr_init(tmp);
			mcon_str_add(tmp, "Host does not match replicaset name. Expected: ", 0);
			mcon_str_add(tmp, *repl_set_name, 0);
			mcon_str_add(tmp, "; Found: ", 0);
			mcon_str_add(tmp, set, 0);

			*error_message = strdup(tmp->d);
			mcon_str_ptr_dtor(tmp);

			free(data_buffer);
			return 0;
		} else {
			mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "rs_status: the found replicaset name matches the expected one (%s).", set);
		}
	} else if (*repl_set_name == NULL) {
		/* This can not happen, as for the REPLSET CON_TYPE to be active in the
		 * first place, there needs to be a repl_set_name set. */
		mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "rs_status: the replicaset name is not set, so we're using %s.", set);
		*repl_set_name = strdup(set);
	}

	/* Find all hosts */
	bson_find_field_as_array(ptr, "members", &hosts);
	*nr_hosts = 0;
	do {
		char          *name;
		int32_t        state = 0;
		unsigned char  self = 0;

		member_ptr = hosts;
		member_ptr = bson_skip_field_name(member_ptr);
		member_ptr = member_ptr + sizeof(int32_t); /* Skip the length */

		bson_find_field_as_int32(member_ptr, "state", &state);
		bson_find_field_as_string(member_ptr, "name", &name);
		bson_find_field_as_bool(member_ptr, "self", &self);

		if (self) {
			we_think_we_are = mongo_server_hash_to_server(con->hash);
			if (strcmp(name, we_think_we_are) == 0) {
				mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "rs_status: the server name matches what we thought it'd be (%s).", we_think_we_are);
			} else {
				mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "rs_status: the server name (%s) did not match with what we thought it'd be (%s).", name, we_think_we_are);
				/* We reset the name as the server responded with a different name than
				 * what we thought it was */
				free(server->host);
				server->host = strndup(name, strchr(name, ':') - name);
				server->port = atoi(strchr(name, ':') + 1);
				retval = 3;
			}
			free(we_think_we_are);

			/* We only use the state in case we're "self", the other servers
			 * will get this updated when they get hit during the rs_status()
			 * call on it. */
			switch (state) {
				case MONGO_STATE_PRIMARY:
					con->connection_type = MONGO_NODE_PRIMARY;
					break;
				case MONGO_STATE_SECONDARY:
					con->connection_type = MONGO_NODE_SECONDARY;
					break;
				case MONGO_STATE_ARBITER:
					con->connection_type = MONGO_NODE_ARBITER;
					break;
			}
		}

		switch (state) {
			case MONGO_STATE_PRIMARY:
			case MONGO_STATE_SECONDARY:
				(*nr_hosts)++;
				*found_hosts = realloc(*found_hosts, (*nr_hosts) * sizeof(char*));
				(*found_hosts)[*nr_hosts-1] = strdup(name);
				mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "rs_status: found a connectable host: %s (state: %d)", name, state);
				break;

			case MONGO_STATE_ARBITER:
				mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "rs_status: found an arbiter host: %s (state: %d)", name, state);
				break;

			default:
				mongo_manager_log(manager, MLOG_CON, MLOG_WARN, "rs_status: found an unconnectable host: %s (state: %d)", name, state);
				break;
		}
	} while (bson_array_find_next_embedded_doc(&hosts));

	free(data_buffer);

	con->last_is_master = now.tv_sec;
	mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "rs_status: last ran at %ld", con->last_is_master);

	return retval;
}

/**
 * Sends an is_master command to the server to find server flags
 *
 * Returns 1 when it worked, and 0 when an error was encountered.
 */
int mongo_connection_get_server_flags(mongo_con_manager *manager, mongo_connection *con, char **error_message)
{
	mcon_str      *packet;
	int32_t        max_bson_size = 0;
	char          *data_buffer;
	char          *ptr;
	char          *tags;
	char          *msg; /* If set and its value is "isdbgrid", it signals we connected to a mongos */

	mongo_manager_log(manager, MLOG_CON, MLOG_INFO, "get_server_flags: start");
	packet = bson_create_is_master_packet(con);

	if (!mongo_connect_send_packet(manager, con, packet, &data_buffer, error_message)) {
		return 0;
	}

	/* Find data fields */
	ptr = data_buffer + sizeof(int32_t); /* Skip the length */

	/* Find max bson size */
	if (bson_find_field_as_int32(ptr, "maxBsonObjectSize", &max_bson_size)) {
		mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "get_server_flags: setting maxBsonObjectSize to %d", max_bson_size);
		con->max_bson_size = max_bson_size;
	} else {
		*error_message = strdup("Couldn't find the maxBsonObjectSize field");
		free(data_buffer);
		return 0;
	}

	/* Find msg and whether it contains "isdbgrid" */
	if (bson_find_field_as_string(ptr, "msg", (char**) &msg)) {
		if (strcmp(msg, "isdbgrid") == 0) {
			mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "get_server_flags: msg contains 'isdbgrid' - we're connected to a mongos");
			con->connection_type = MONGO_NODE_MONGOS;
		}
	}

	/* Find read preferences tags */
	con->tag_count = 0;
	con->tags = NULL;
	if (bson_find_field_as_document(ptr, "tags", (char**) &tags)) {
		char *it, *name, *value;
		int   length;

		it = tags;

		while (bson_array_find_next_string(&it, &name, &value)) {
			con->tags = realloc(con->tags, (con->tag_count + 1) * sizeof(char*));
			length = strlen(name) + strlen(value) + 2;
			con->tags[con->tag_count] = malloc(length);
			snprintf(con->tags[con->tag_count], length, "%s:%s", name, value);
			free(name);
			mongo_manager_log(manager, MLOG_CON, MLOG_FINE, "get_server_flags: added tag %s", con->tags[con->tag_count]);
			con->tag_count++;
		}
	}

	free(data_buffer);

	return 1;
}
