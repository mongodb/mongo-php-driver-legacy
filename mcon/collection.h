#ifndef __MCON_COLLECTION_H__
#define __MCON_COLLECTION_H__

typedef struct _mcon_collection
{
	int count;
	int space;
	int data_size;
	void **data;
} mcon_collection;

typedef void (mcon_collection_callback_t)(void *elem);

mcon_collection *mcon_init_collection(int data_size);
void mcon_collection_add(mcon_collection *c, void *data);
void mcon_collection_iterate(mcon_collection *c, mcon_collection_callback_t cb);
void mcon_collection_free(mcon_collection *c);
#endif
