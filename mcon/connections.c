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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define INT_32  4
#define FLAGS   0

#define MONGO_REPLY_FLAG_QUERY_FAILURE 0x02

/* Helper functions */
inline int mongo_connection_get_reqid(mongo_connection *con)
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

mongo_connection *mongo_connection_create(mongo_server_def *server_def)
{
	mongo_connection *tmp;
	char             *error_message = NULL;

	/* Init struct */
	tmp = malloc(sizeof(mongo_connection));
	memset(tmp, 0, sizeof(mongo_connection));
	tmp->last_reqid = rand();

	tmp->socket = mongo_connection_connect(server_def->host, server_def->port, 1000, &error_message);
	if (tmp->socket == -1) {
		printf("ERROR: %s\n", error_message);
		free(tmp);
		return NULL;
	}

	return tmp;
}

void mongo_connection_destroy(mongo_connection *con)
{
	free(con);
}

#define MONGO_REPLY_HEADER_SIZE 36

/**
 * Sends a ping command to the server and stores the result.
 *
 * Returns 1 when it worked, and 0 when an error was encountered.
 */
int mongo_connection_ping(mongo_connection *con)
{
	mcon_str      *packet;
	char          *error_message = NULL;
	int            len, read;
	struct timeval start, end;
	char           reply_buffer[MONGO_REPLY_HEADER_SIZE];
	uint32_t       flags; /* To check for query reply status */

	packet = bson_create_ping_packet(con);

	gettimeofday(&start, NULL);

	/* Send and wait for reply */
	mongo_io_send(con->socket, packet->d, packet->l, &error_message);
	mcon_str_ptr_dtor(packet);
	read = mongo_io_recv_header(con->socket, reply_buffer, MONGO_REPLY_HEADER_SIZE, &error_message);

	/* If the header too small? */
	if (read < 5 * sizeof(int32_t)) {
		return 0;
	}
	/* Check for a query error */
	flags = MONGO_32(*(int*)(reply_buffer + sizeof(int32_t) * 4));
	if (flags & MONGO_REPLY_FLAG_QUERY_FAILURE) {
		return 0;
	}

	gettimeofday(&end, NULL);

	con->last_ping = end.tv_sec;
	con->ping_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
	if (con->ping_ms < 0) { /* some clocks do weird stuff */
		con->ping_ms = 0;
	}
	printf("PING: TS: %d = %d\n", con->last_ping, con->ping_ms);

	return 1;
}
