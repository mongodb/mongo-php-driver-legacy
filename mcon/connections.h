#ifndef __MCON_CONNECTION_H__
#define __MCON_CONNECTION_H__

#include "types.h"

mongo_connection *mongo_connection_create(mongo_server_def *server_def);

inline int mongo_connection_get_reqid(mongo_connection *con);
int mongo_connection_ping(mongo_connection *con);

#endif
