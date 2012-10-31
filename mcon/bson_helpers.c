#include "types.h"
#include "bson_helpers.h"
#include "str.h"

void mcon_serialize_int(struct mcon_str *str, int num)
{
	int i = MONGO_32(num);
	mcon_str_addl(str, (char*) &i, 4, 0);
}

void mcon_serialize_int32(struct mcon_str *str, int num)
{
	int i = MONGO_32(num);
	mcon_str_addl(str, (char*) &i, 4, 0);
}

void mcon_serialize_int64(struct mcon_str *str, int64_t num)
{
	int64_t i = MONGO_64(num);
	mcon_str_addl(str, (char*) &i, 8, 0);
}
