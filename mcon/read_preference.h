#ifndef __MCON_READ_PREFERENCE_H__
#define __MCON_READ_PREFERENCE_H__

#include "types.h"
#include "collection.h"

#define MONGO_RP_FIRST               0x00

#define MONGO_RP_PRIMARY             0x00
#define MONGO_RP_PRIMARY_PREFERRED   0x01
#define MONGO_RP_SECONDARY           0x02
#define MONGO_RP_SECONDARY_PREFERRED 0x03
#define MONGO_RP_NEAREST             0x04

#define MONGO_RP_LAST                0x04


/* TODO: Needs to be a setting through the connection string/options */
#define MONGO_RP_CUTOFF  15

typedef int (mongo_connection_sort_t)(const void *a, const void *b);

mcon_collection* mongo_find_candidate_servers(mongo_con_manager *manager, mongo_read_preference *rp);
mcon_collection *mongo_sort_servers(mongo_con_manager *manager, mcon_collection *col, mongo_read_preference *rp);
mcon_collection *mongo_select_nearest_servers(mongo_con_manager *manager, mcon_collection *col, mongo_read_preference *rp);
mongo_connection *mongo_pick_server_from_set(mongo_con_manager *manager, mcon_collection *col, mongo_read_preference *rp);

/* RP helpers */
char *mongo_read_preference_type_to_name(int type);
char *mongo_read_preference_squash_tagset(mongo_read_preference_tagset *tagset);

void mongo_read_preference_add_tag(mongo_read_preference_tagset *tagset, char *name, char *value);
void mongo_read_preference_add_tagset(mongo_read_preference *rp, mongo_read_preference_tagset *tagset);

void mongo_read_preference_tagset_dtor(mongo_read_preference_tagset *tagset);
void mongo_read_preference_dtor(mongo_read_preference *rp);

void mongo_read_preference_copy(mongo_read_preference *from, mongo_read_preference *to);
void mongo_read_preference_replace(mongo_read_preference *from, mongo_read_preference *to);

#endif
