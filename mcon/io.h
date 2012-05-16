#ifndef __MCON_IO_H__
#define __MCON_IO_H__

int mongo_io_send(int sock, char *packet, int total, char **error_message);

#endif
