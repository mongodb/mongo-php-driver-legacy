#ifndef __MCON_MINI_BSON_H__
#define __MCON_MINI_BSON_H__

#include "types.h"

mcon_str *bson_create_ping_packet(mongo_connection *con);
mcon_str *bson_create_is_master_packet(mongo_connection *con);

int bson_find_field_as_array(char *buffer, char *field, char **data);
int bson_find_field_as_bool(char *buffer, char *field, unsigned char *data);
int bson_find_field_as_string(char *buffer, char *field, char **data);

#endif
