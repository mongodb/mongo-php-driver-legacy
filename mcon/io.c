#include <string.h>
#include <errno.h>

/*
 * Low-level send function.
 *
 * Goes through the buffer sending 4K byte batches.
 * On failure, sets errmsg to errno string and returns -1.
 * On success, returns number of bytes sent.
 * Does not attempt to reconnect nor throw any exceptions.
 *
 * On failure, the calling function is responsible for disconnecting
 */
int mongo_io_send(int sock, char *packet, int total, char **error_message)
{
	int sent = 0, status = 1;

	while (sent < total && status > 0) {
		int len = 4096 < (total - sent) ? 4096 : total - sent;

		status = send(sock, (const char*)packet + sent, len, 0);

		if (status == -1) {
			*error_message = strdup(strerror(errno));
			return -1;
		}
		sent += status;
	}

	return sent;
}

/*
 * Low-level receive function.
 *
 * On failure, sets errmsg to errno string and returns -1.
 * On success, returns number of bytes read.
 * Does not attempt to reconnect nor throw any exceptions.
 *
 * On failure, the calling function is responsible for disconnecting
 */
int mongo_io_recv_header(int sock, char *reply_buffer, int size, char **error_message)
{
	int status;

	status = recv(sock, reply_buffer, size, 0);

	if (status == -1) {
		*error_message = strdup(strerror(errno));
		return -1;
	} else if (status == 0) {
		*error_message = strdup("The socket is closed");
		return -1;
	}
	return status;
}
