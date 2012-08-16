#ifndef __MCON_MINI_BSON_H__
#define __MCON_MINI_BSON_H__

#include "types.h"

mcon_str *bson_create_ping_packet(mongo_connection *con);
mcon_str *bson_create_is_master_packet(mongo_connection *con);
mcon_str *bson_create_rs_status_packet(mongo_connection *con);

char *bson_skip_field_name(char *data);
int bson_find_field_as_array(char *buffer, char *field, char **data);
int bson_find_field_as_document(char *buffer, char *field, char **data);
int bson_find_field_as_bool(char *buffer, char *field, unsigned char *data);
int bson_find_field_as_int32(char *buffer, char *field, int32_t *data);
int bson_find_field_as_string(char *buffer, char *field, char **data);

int bson_array_find_next_string(char **buffer, char **field, char **data);
int bson_array_find_next_embedded_doc(char **buffer);

#endif
