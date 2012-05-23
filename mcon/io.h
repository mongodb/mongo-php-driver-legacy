#ifndef __MCON_IO_H__
#define __MCON_IO_H__

int mongo_io_send(int sock, char *packet, int total, char **error_message);
int mongo_io_recv_header(int sock, char *reply_buffer, int size, char **error_message);
char *mongo_io_recv_data(int sock, void *dest, int size, char **error_message);

#endif
