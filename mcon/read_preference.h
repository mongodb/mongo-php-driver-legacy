#ifndef __MCON_READ_PREFERENCE_H__
#define __MCON_READ_PREFERENCE_H__

#include "collection.h"

#define MONGO_RP_PRIMARY        0x01
#define MONGO_RP_SECONDARY      0x02
#define MONGO_RP_SECONDARY_ONLY 0x03
#define MONGO_RP_ANY            0x04

typedef struct _mongo_read_preference
{
	int type; /* MONGO_RP_* */
} mongo_read_preference;

typedef int (mongo_connection_sort_t)(const void *a, const void *b);

mcon_collection *mongo_rp_collect_primary(mongo_con_manager *manager);
mcon_collection *mongo_rp_collect_secondary(mongo_con_manager *manager);
mcon_collection *mongo_rp_collect_secondary_only(mongo_con_manager *manager);
mcon_collection *mongo_rp_collect_any(mongo_con_manager *manager);

#endif
