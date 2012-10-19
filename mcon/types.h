#ifndef __MCON_TYPES_H__
#define __MCON_TYPES_H__

#include <time.h>
#include <stdarg.h>

#define MONGO_CON_TYPE_STANDALONE 1
#define MONGO_CON_TYPE_MULTIPLE   2
#define MONGO_CON_TYPE_REPLSET    3

/* Bitfield used for fetching connections */
#define MONGO_CON_FLAG_READ            0x01
#define MONGO_CON_FLAG_WRITE           0x02
#define MONGO_CON_FLAG_DONT_CONNECT    0x04

/* These constants are a bit field - however, each connection will only have
 * one type. The reason why it's a bit field is because of filtering during
 * read preference scanning (see read_preference.c).
 *
 * SECONDARY needs to have a larger constant value than PRIMARY for the read
 * preference sorting algorithm to work. */
#define MONGO_NODE_STANDALONE     0x01
#define MONGO_NODE_PRIMARY        0x02
#define MONGO_NODE_SECONDARY      0x04
#define MONGO_NODE_ARBITER        0x08
#define MONGO_NODE_MONGOS         0x10

/* Constants for health states as returned by replSetGetStatus
 * From: http://docs.mongodb.org/manual/reference/replica-status/#statuses */
#define MONGO_STATE_STARTING1     0x00
#define MONGO_STATE_PRIMARY       0x01
#define MONGO_STATE_SECONDARY     0x02
#define MONGO_STATE_RECOVERING    0x03
#define MONGO_STATE_FATAL_ERROR   0x04
#define MONGO_STATE_STARTING2     0x05
#define MONGO_STATE_UNKNOWN       0x06
#define MONGO_STATE_ARBITER       0x07
#define MONGO_STATE_DOWN          0x08
#define MONGO_STATE_ROLLBACK      0x09
#define MONGO_STATE_REMOVED       0x0a

/*
 * Constants for the logging framework
 */
/* Levels */
#define MLOG_WARN    1
#define MLOG_INFO    2
#define MLOG_FINE    4

/* Modules */
#define MLOG_RS      1
#define MLOG_CON     2
#define MLOG_IO      4
#define MLOG_SERVER  8
#define MLOG_PARSE  16

#define MLOG_NONE    0
#define MLOG_ALL    31 /* Must be the bit sum of all above */


/* Stores all the information about the connection. The hash is a group of
 * parameters to identify a unique connection. */

typedef int (mongo_cleanup_t)(void *callback_data);
typedef struct _mongo_connection_deregister_callback {
  void *callback_data;
  mongo_cleanup_t *mongo_cleanup_cb;
  struct _mongo_connection_deregister_callback *next;
} mongo_connection_deregister_callback;

typedef struct _mongo_connection
{
	time_t last_ping; /* The timestamp when ping was called last */
	int    ping_ms;
	int    last_ismaster; /* The timestamp when ismaster/get_server_flags was called last */
	int    last_reqid;
	int    socket;
	int    connection_type; /* MONGO_NODE_: PRIMARY, SECONDARY, ARBITER, MONGOS */
	int    max_bson_size; /* Store per connection, as it can actually differ */
	int    tag_count;
	char **tags;
	char  *hash; /* Duplicate of the hash that the manager knows this connection as */
	mongo_connection_deregister_callback *cleanup_list;
} mongo_connection;

typedef struct _mongo_con_manager_item
{
	char                           *hash;
	mongo_connection               *connection;
	struct _mongo_con_manager_item *next;
} mongo_con_manager_item;

typedef void (mongo_log_callback_t)(int module, int level, void *context, char *format, va_list arg);

#define MONGO_MANAGER_DEFAULT_PING_INTERVAL    5
#define MONGO_MANAGER_DEFAULT_MASTER_INTERVAL 15

typedef struct _mongo_con_manager
{
	mongo_con_manager_item *connections;

	/* context and callback function that is used to send logging information
	 * through */
	void                   *log_context;
	mongo_log_callback_t   *log_function;

	/* ping/ismaster will not be called more often than the amount of seconds that
	 * is configured with ping_interval/ismaster_interval. The ismaster interval
	 * is also used for the get_server_flags function. */
	long                    ping_interval;      /* default:  5 seconds */
	long                    ismaster_interval;  /* default: 15 seconds */
} mongo_con_manager;

typedef struct _mongo_read_preference_tagset
{
	int    tag_count;
	char **tags;
} mongo_read_preference_tagset;

typedef struct _mongo_read_preference
{
	int                            type; /* MONGO_RP_* */
	int                            tagset_count; /* The number of tag sets in this RP */
	mongo_read_preference_tagset **tagsets;
} mongo_read_preference;

typedef struct _mongo_server_def
{
	char *host;
	int   port;
	char *db;
	char *username;
	char *password;
} mongo_server_def;

typedef struct _mongo_servers
{
	int               count;
	mongo_server_def *server[16]; /* TODO: Make this dynamic */

	/* flags and options */
	int                   con_type;
	char                 *repl_set_name;
	int                   connectTimeoutMS;
	int                   default_fire_and_forget; /* Sets a flag whether all operations should be "safe" by default or not, -1 is not set, where 0 and 1 are explicit sets. */

	mongo_read_preference read_pref;
} mongo_servers;

typedef struct _mcon_collection
{
	int count;
	int space;
	int data_size;
	void **data;
} mcon_collection;

typedef void (mcon_collection_callback_t)(mongo_con_manager *manager, void *elem);

#endif
