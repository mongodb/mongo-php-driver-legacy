#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "str.h"
#include "mini_bson.h"
#include "types.h"

static mcon_str *create_simple_header(mongo_connection *con)
{
	struct mcon_str *str;

	mcon_str_ptr_init(str);

	mcon_serialize_int(str, 0); /* We need to fill this with the length */

	mcon_serialize_int(str, mongo_connection_get_reqid(con));
	mcon_serialize_int(str, 0); /* Reponse to */
	mcon_serialize_int(str, 2004); /* OP_QUERY */

	mcon_serialize_int(str, 0); /* Flags */
	mcon_str_addl(str, "admin.$cmd", 11, 0);
	mcon_serialize_int(str, 0); /* Number to skip */
	mcon_serialize_int(str, 0); /* Number to return */

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
