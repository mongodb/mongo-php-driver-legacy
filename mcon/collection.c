#include "collection.h"
#include <stdlib.h>

mcon_collection *mcon_init_collection(int data_size)
{
	mcon_collection *c;

	c = malloc(sizeof(mcon_collection));
	c->count = 0;
	c->space = 16;
	c->data_size = data_size;
	c->data = malloc(c->space * c->data_size);

	return c;
}

void mcon_collection_add(mcon_collection *c, void *data)
{
	if (c->count == c->space) {
		c->space = c->space * 2;
		c->data = realloc(c->data, c->space * c->data_size);
	}
	c->data[c->count] = data;
	c->count++;
}

void mcon_collection_iterate(mongo_con_manager *manager, mcon_collection *c, mcon_collection_callback_t cb)
{
	int i;

	for (i = 0; i < c->count; i++) {
		cb(manager, c->data[i]);
	}
}

void mcon_collection_free(mcon_collection *c)
{
	free(c->data);
	free(c);
}
