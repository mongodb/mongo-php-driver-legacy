/**
 *  Copyright 2009-2012 10gen, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef __MCON_MANAGER_H__
#define __MCON_MANAGER_H__

#include "types.h"
#include "read_preference.h"

/* Manager */
mongo_con_manager *mongo_init(void);
void mongo_deinit(mongo_con_manager *manager);

/* Fetching connections */
/* connection_flags: Bitfield consisting of MONGO_CON_FLAG_READ/MONGO_CON_FLAG_WRITE/MONGO_CON_FLAG_DONT_CONNECT */
mongo_connection *mongo_get_read_write_connection(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, char **error_message);
mongo_connection *mongo_get_read_write_connection_with_callback(mongo_con_manager *manager, mongo_servers *servers, int connection_flags, void *callback_data, mongo_cleanup_t cleanup_cb, char **error_message);

/* Connection management */
mongo_connection *mongo_manager_connection_find_by_hash(mongo_con_manager *manager, char *hash);
void mongo_manager_connection_register(mongo_con_manager *manager, mongo_connection *con);
int mongo_manager_connection_deregister(mongo_con_manager *manager, mongo_connection *con);
int mongo_deregister_callback_from_connection(mongo_connection *connection, void *cursor);
/* Connection blacklisting */
void mongo_manager_connection_blacklist_add(mongo_con_manager *manager, char *hash);
int mongo_manager_connection_blacklist_search(mongo_con_manager *manager, char *hash);

/* Logging */
void mongo_log_null(int module, int level, void *context, char *format, va_list arg);
void mongo_log_printf(int module, int level, void *context, char *format, va_list arg);
void mongo_manager_log(mongo_con_manager *manager, int module, int level, char *format, ...);
#endif
