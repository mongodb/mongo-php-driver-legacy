#ifndef __MCON_MANAGER_H__
#define __MCON_MANAGER_H__

#include "types.h"
#include "read_preference.h"

/* Manager */
mongo_con_manager *mongo_init(void);
void mongo_deinit(mongo_con_manager *manager);

/* Fetching connections */
int mongo_deregister_cursor_from_connection(mongo_connection *connection, void *cursor);
mongo_connection *mongo_get_read_write_connection_for_cursor(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, void *callback_data, mongo_cleanup_t cleanup_cb, char **error_message);
/* connection_flags: Bitfield consisting of MONGO_CON_FLAG_READ/MONGO_CON_FLAG_WRITE/MONGO_CON_FLAG_DONT_CONNECT */
mongo_connection *mongo_get_read_write_connection(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, char **error_message);

/* Connection management */
mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash);
void mongo_manager_connection_register(mongo_con_manager *manager, mongo_connection *con);
int mongo_manager_connection_deregister(mongo_con_manager *manager, mongo_connection *con);

/* Logging */
void mongo_log_null(int module, int level, void *context, char *format, va_list arg);
void mongo_log_printf(int module, int level, void *context, char *format, va_list arg);
void mongo_manager_log(mongo_con_manager *manager, int module, int level, char *format, ...);
#endif
