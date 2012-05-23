#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "str.h"
#include "mini_bson.h"
#include "types.h"
#include "bson_helpers.h"

#define MONGO_QUERY_FLAG_EMPTY    0x00
#define MONGO_QUERY_FLAG_SLAVE_OK 0x04

static mcon_str *create_simple_header(mongo_connection *con)
{
	struct mcon_str *str;

	mcon_str_ptr_init(str);

	mcon_serialize_int(str, 0); /* We need to fill this with the length */

	mcon_serialize_int(str, mongo_connection_get_reqid(con));
	mcon_serialize_int(str, 0); /* Reponse to */
	mcon_serialize_int(str, 2004); /* OP_QUERY */

	mcon_serialize_int(str, MONGO_QUERY_FLAG_SLAVE_OK); /* Flags */
	mcon_str_addl(str, "admin.$cmd", 11, 0);
	mcon_serialize_int(str, 0); /* Number to skip */
	mcon_serialize_int(str, -1); /* Number to return, has to be -1 for admin commands */

	return str;
}

void bson_add_long(mcon_str *str, char *fieldname, int64_t v)
{
	mcon_str_addl(str, "\x12", 1, 0);
	mcon_str_addl(str, fieldname, strlen(fieldname) + 1, 0);
	mcon_serialize_int64(str, v);
}

mcon_str *bson_create_ping_packet(mongo_connection *con)
{
	struct mcon_str *str = create_simple_header(con);
	int    hdr;

	hdr = str->l;
	mcon_serialize_int(str, 0); /* We need to fill this with the length */
	bson_add_long(str, "ping", 1);
	mcon_str_addl(str, "", 1, 0); /* Trailing 0x00 */

	/* Set length */
	((int*) (&(str->d[hdr])))[0] = str->l - hdr;

	((int*) str->d)[0] = str->l;
	return str;
}

mcon_str *bson_create_is_master_packet(mongo_connection *con)
{
	struct mcon_str *str = create_simple_header(con);
	int    hdr;

	hdr = str->l;
	mcon_serialize_int(str, 0); /* We need to fill this with the length */
	bson_add_long(str, "isMaster", 1);
	mcon_str_addl(str, "", 1, 0); /* Trailing 0x00 */

	/* Set length */
	((int*) (&(str->d[hdr])))[0] = str->l - hdr;

	((int*) str->d)[0] = str->l;
	return str;
}

/* Field reading functionality */
/* - helpers */
char *bson_skip_field_name(char *data)
{
	return strchr(data, '\0') + 1;
}

#define BSON_DOUBLE          0x01
#define BSON_STRING          0x02
#define BSON_DOCUMENT        0x03
#define BSON_ARRAY           0x04
#define BSON_BINARY          0x05
#define BSON_UNDEFINED       0x06
#define BSON_OBJECT_ID       0x07
#define BSON_BOOLEAN         0x08
#define BSON_DATETIME        0x09
#define BSON_NULL            0x0A
#define BSON_REGEXP          0x0B
#define BSON_DBPOINTER       0x0C
#define BSON_JAVASCRIPT      0x0D
#define BSON_SYMBOL          0x0E
#define BSON_JAVASCRIPT_WITH_SCOPE 0x0F
#define BSON_INT32           0x10
#define BSON_TIMESTAMP       0x11
#define BSON_INT64           0x12
#define BSON_MIN_KEY         0xFF
#define BSON_MAX_KEY         0x7F

char *bson_next(char *data)
{
	unsigned char type = data[0];
	char         *name;
	int32_t       length;

	if (type == 0) {
		return NULL;
	}

	data = bson_skip_field_name(data + 1); /* Skip 1, because of the type in data[0] */
/*
element 	::= 	"\x01" e_name double 	Floating point
	| 	"\x02" e_name string 	UTF-8 string
	| 	"\x03" e_name document 	Embedded document
	| 	"\x04" e_name document 	Array
	| 	"\x05" e_name binary 	Binary data
	| 	"\x06" e_name 	Undefined — Deprecated
	| 	"\x07" e_name (byte*12) 	ObjectId
	| 	"\x08" e_name "\x00" 	Boolean "false"
	| 	"\x08" e_name "\x01" 	Boolean "true"
	| 	"\x09" e_name int64 	UTC datetime
	| 	"\x0A" e_name 	Null value
	| 	"\x0B" e_name cstring cstring 	Regular expression
	| 	"\x0C" e_name string (byte*12) 	DBPointer — Deprecated
	| 	"\x0D" e_name string 	JavaScript code
	| 	"\x0E" e_name string 	Symbol
	| 	"\x0F" e_name code_w_s 	JavaScript code w/ scope
	| 	"\x10" e_name int32 	32-bit Integer
	| 	"\x11" e_name int64 	Timestamp
	| 	"\x12" e_name int64 	64-bit integer
	| 	"\xFF" e_name 	Min key
	| 	"\x7F" e_name 	Max key
*/
	switch (type) {
		case BSON_DOUBLE:
			return data + sizeof(double);
		case BSON_STRING:
		case BSON_JAVASCRIPT:
		case BSON_SYMBOL:
			length = MONGO_32(*(int*)data);
			return data + sizeof(int32_t) + length;
		case BSON_DOCUMENT:
		case BSON_ARRAY:
			length = MONGO_32(*(int*)data);
			return data + length;
		case BSON_BINARY:
			length = MONGO_32(*(int*)data);
			return data + sizeof(int32_t) + 1 + length;
		case BSON_UNDEFINED:
		case BSON_NULL:
		case BSON_MIN_KEY:
		case BSON_MAX_KEY:
			return data;
		case BSON_OBJECT_ID:
			return data + 12;
		case BSON_BOOLEAN:
			return data + 1;
		case BSON_DATETIME:
		case BSON_TIMESTAMP:
		case BSON_INT64:
			return data + sizeof(int64_t);
		case BSON_REGEXP:
			return strchr(data, '\0') + 1;
		case BSON_DBPOINTER:
			length = MONGO_32(*(int*)data);
			return data + sizeof(int32_t) + length + 12;
		case BSON_JAVASCRIPT_WITH_SCOPE:
			exit(-3); /* TODO */
		case BSON_INT32:
			return data + sizeof(int32_t);
	}
}

void *bson_get_current(char *data, char **field_name, int *type)
{
	*type = data[0];
	*field_name = data + 1; /* Skip 1, because of the type in data[0] */
	data = bson_skip_field_name(data); /* Skip fieldname to get to data */
	return data;
}

void *bson_find_field(char *data, char *field_name, int type)
{
	void *return_data;
	char *ptr = data;
	char *read_field;
	int   read_type;

	return_data = bson_get_current(ptr, &read_field, &read_type);
	printf("read_field: %s\n", read_field);
	while (strcmp(read_field, field_name) != 0 || read_type != type) {
		printf("read_field: %s\n", read_field);
		ptr = bson_next(ptr);
		if (ptr == NULL) {
			read_field = NULL;
			break;
		}
		return_data = bson_get_current(ptr, &read_field, &read_type);
	}
	if (read_field && strcmp(read_field, field_name) == 0 && read_type == type) {
		return return_data;
	}
}

/* - Public API */
int bson_find_field_as_array(char *buffer, char *field, char **data)
{
	char* tmp = bson_find_field(buffer, field, BSON_ARRAY) + 4; /* int32 for length */

	if (tmp) {
		*data = tmp;
		return 1;
	}
	return 0;
}

int bson_find_field_as_bool(char *buffer, char *field, unsigned char *data)
{
	char *tmp = (char*) bson_find_field(buffer, field, BSON_BOOLEAN);

	if (tmp) {
		*data = tmp[0];
		return 1;
	}
	return 0;
}

int bson_find_field_as_string(char *buffer, char *field, char **data)
{
	char* tmp = bson_find_field(buffer, field, BSON_STRING) + 4; /* int32 for length */

	if (tmp) {
		*data = tmp;
		return 1;
	}
	return 0;
}

int bson_array_find_next_string(char **buffer, char **data)
{
	char *read_field;
	int   read_type;
	void *return_data;

	return_data = bson_get_current(*buffer, &read_field, &read_type);
	if (read_type == BSON_STRING) {
		*data = return_data + 4;
	}
	*buffer = bson_next(*buffer);
	return *buffer == NULL ? 0 : 1;
}
