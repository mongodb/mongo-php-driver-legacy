
#ifndef PHP_MONGO_H
#define PHP_MONGO_H 1

#define PHP_MONGO_VERSION "1.0"
#define PHP_MONGO_EXTNAME "mongo"

#define PHP_DB_CLIENT_CONNECTION_RES_NAME "mongo connection"

PHP_MINIT_FUNCTION(mongo);

/*// from mysql, non-applicable ones commented out
//PHP_FUNCTION(mongo_affected_rows);
PHP_FUNCTION(mongo_change_user);
PHP_FUNCTION(mongo_client_encoding);
PHP_FUNCTION(mongo_create_db);
//PHP_FUNCTION(mongo_data_seek);
PHP_FUNCTION(mongo_db_name);
PHP_FUNCTION(mongo_db_query);
PHP_FUNCTION(mongo_drop_db);
//PHP_FUNCTION(mongo_errno);
//PHP_FUNCTION(mongo_error);
PHP_FUNCTION(mongo_fetch_array);
PHP_FUNCTION(mongo_fetch_assoc);
PHP_FUNCTION(mongo_fetch_field);
PHP_FUNCTION(mongo_fetch_lengths);
PHP_FUNCTION(mongo_fetch_object);
PHP_FUNCTION(mongo_fetch_row);
//PHP_FUNCTION(mongo_field_flags);
PHP_FUNCTION(mongo_field_len);
PHP_FUNCTION(mongo_field_name);
PHP_FUNCTION(mongo_field_seek);
PHP_FUNCTION(mongo_field_table);
PHP_FUNCTION(mongo_field_type);
PHP_FUNCTION(mongo_free_result);
PHP_FUNCTION(mongo_get_client_info);
PHP_FUNCTION(mongo_get_host_info);
PHP_FUNCTION(mongo_get_proto_info);
PHP_FUNCTION(mongo_get_server_info);
PHP_FUNCTION(mongo_info);
PHP_FUNCTION(mongo_insert_id);
//PHP_FUNCTION(mongo_list_dbs);
//PHP_FUNCTION(mongo_list_fields);
//PHP_FUNCTION(mongo_list_processes);
PHP_FUNCTION(mongo_list_tables);
//PHP_FUNCTION(mongo_num_fields);
PHP_FUNCTION(mongo_num_rows);
PHP_FUNCTION(mongo_pconnect);
PHP_FUNCTION(mongo_ping);
PHP_FUNCTION(mongo_query);
PHP_FUNCTION(mongo_real_escape_string);
PHP_FUNCTION(mongo_result);
PHP_FUNCTION(mongo_select_db);
PHP_FUNCTION(mongo_set_charset);
PHP_FUNCTION(mongo_stat);
PHP_FUNCTION(mongo_tablename);
//PHP_FUNCTION(mongo_thread_id);
//PHP_FUNCTION(mongo_unbuffered_query);
*/

PHP_FUNCTION(mongo_connect);
PHP_FUNCTION(mongo_close);
PHP_FUNCTION(mongo_ensure_index);
PHP_FUNCTION(mongo_find);
PHP_FUNCTION(mongo_find_one);
PHP_FUNCTION(mongo_has_next);
PHP_FUNCTION(mongo_kill_cursors);
PHP_FUNCTION(mongo_next);
PHP_FUNCTION(mongo_remove);
PHP_FUNCTION(mongo_insert);
PHP_FUNCTION(mongo_update);
PHP_FUNCTION(mongo_limit);
PHP_FUNCTION(mongo_sort);

PHP_FUNCTION(temp);

extern zend_module_entry mongo_module_entry;
#define phpext_mongo_ptr &mongo_module_entry

#endif
