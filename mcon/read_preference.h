#ifndef __MCON_READ_PREFERENCE_H__
#define __MCON_READ_PREFERENCE_H__

#include "collection.h"

#define MONGO_RP_PRIMARY             0x01
#define MONGO_RP_PRIMARY_PREFERRED   0x02
#define MONGO_RP_SECONDARY           0x03
#define MONGO_RP_SECONDARY_PREFERRED 0x04
#define MONGO_RP_NEAREST             0x05

typedef struct _mongo_read_preference
{
	int type; /* MONGO_RP_* */
} mongo_read_preference;

typedef int (mongo_connection_sort_t)(const void *a, const void *b);

mcon_collection* mongo_find_candidate_servers(mongo_con_manager *manager, mongo_read_preference *rp);
mcon_collection *mongo_select_server(mcon_collection *col, mongo_read_preference *rp);


#endif
