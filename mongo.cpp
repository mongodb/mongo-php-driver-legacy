// mongo.cpp
/**
 *  Copyright 2009 10gen, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <vector>

#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#include <mongo/client/dbclient.h>
#include <mongo/client/gridfs.h>

#include "mongo.h"
#include "mongo_types.h"
#include "bson.h"
#include "gridfs.h"

/** Classes */
zend_class_entry *mongo_id_class, 
  *mongo_date_class, 
  *mongo_regex_class, 
  *mongo_bindata_class;

/** Resources */
int le_connection, le_pconnection, le_db_cursor, le_gridfs, le_gridfile;

ZEND_DECLARE_MODULE_GLOBALS(mongo)
static PHP_GINIT_FUNCTION(mongo);

static function_entry mongo_functions[] = {
  PHP_FE( mongo_connect , NULL )
  PHP_FE( mongo_pconnect , NULL )
  PHP_FE( mongo_close , NULL )
  PHP_FE( mongo_remove , NULL )
  PHP_FE( mongo_query , NULL )
  PHP_FE( mongo_find_one , NULL )
  PHP_FE( mongo_insert , NULL )
  PHP_FE( mongo_batch_insert , NULL )
  PHP_FE( mongo_update , NULL )
  PHP_FE( mongo_has_next , NULL )
  PHP_FE( mongo_next , NULL )
  PHP_FE( mongo_gridfs_init , NULL )
  PHP_FE( mongo_gridfs_list , NULL )
  PHP_FE( mongo_gridfs_store , NULL )
  PHP_FE( mongo_gridfs_find , NULL )
  PHP_FE( mongo_gridfile_filename , NULL )
  PHP_FE( mongo_gridfile_size , NULL )
  PHP_FE( mongo_gridfile_write , NULL )
  {NULL, NULL, NULL}
};

static function_entry mongo_id_functions[] = {
  PHP_NAMED_FE( __construct, PHP_FN( mongo_id___construct ), NULL )
  PHP_NAMED_FE( __toString, PHP_FN( mongo_id___toString ), NULL )
  { NULL, NULL, NULL }
};


static function_entry mongo_date_functions[] = {
  PHP_NAMED_FE( __construct, PHP_FN( mongo_date___construct ), NULL )
  PHP_NAMED_FE( __toString, PHP_FN( mongo_date___toString ), NULL )
  { NULL, NULL, NULL }
};


static function_entry mongo_regex_functions[] = {
  PHP_NAMED_FE( __construct, PHP_FN( mongo_regex___construct ), NULL )
  PHP_NAMED_FE( __toString, PHP_FN( mongo_regex___toString ), NULL )
  { NULL, NULL, NULL }
};

static function_entry mongo_bindata_functions[] = {
  PHP_NAMED_FE( __construct, PHP_FN( mongo_bindata___construct ), NULL )
  PHP_NAMED_FE( __toString, PHP_FN( mongo_bindata___toString ), NULL )
  { NULL, NULL, NULL }
};


/* {{{ mongo_module_entry 
 */
zend_module_entry mongo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  PHP_MONGO_EXTNAME,
  mongo_functions,
  PHP_MINIT(mongo),
  PHP_MSHUTDOWN(mongo),
  PHP_RINIT(mongo),
  NULL,
  PHP_MINFO(mongo),
  PHP_MONGO_VERSION,
  PHP_MODULE_GLOBALS(mongo),
  PHP_GINIT(mongo),
  NULL,
  NULL,
  STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_MONGO
ZEND_GET_MODULE(mongo)
#endif


/* {{{ PHP_INI */ 
PHP_INI_BEGIN()
STD_PHP_INI_BOOLEAN("mongo.auto_reconnect", "0", PHP_INI_SYSTEM, OnUpdateLong, auto_reconnect, zend_mongo_globals, mongo_globals) 
STD_PHP_INI_BOOLEAN("mongo.allow_persistent", "1", PHP_INI_SYSTEM, OnUpdateLong, allow_persistent, zend_mongo_globals, mongo_globals) 
STD_PHP_INI_ENTRY_EX("mongo.max_persistent", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_persistent, zend_mongo_globals, mongo_globals, display_link_numbers) 
STD_PHP_INI_ENTRY_EX("mongo.max_connections", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_links, zend_mongo_globals, mongo_globals, display_link_numbers) 
STD_PHP_INI_ENTRY("mongo.default_host", NULL, PHP_INI_ALL, OnUpdateString, default_host, zend_mongo_globals, mongo_globals) 
STD_PHP_INI_ENTRY("mongo.default_port", "27017", PHP_INI_ALL, OnUpdateLong, default_port, zend_mongo_globals, mongo_globals) 
PHP_INI_END()
/* }}} */


/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mongo){
  mongo_globals->num_persistent = 0; 
  mongo_globals->num_links = 0; 
  mongo_globals->auto_reconnect = false; 
  mongo_globals->default_host = "localhost"; 
  mongo_globals->default_port = 27017; 
}
/* }}} */


static void php_connection_dtor( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo::DBClientConnection *conn = (mongo::DBClientConnection*)rsrc->ptr;
  if (rsrc->type == le_pconnection) {
    MonGlo(num_persistent)--;
  }
  if( conn )
    delete conn;
  MonGlo(num_links)--;
}

static void php_gridfs_dtor( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo::GridFS *fs = (mongo::GridFS*)rsrc->ptr;
  if( fs )
    delete fs;
}


static void php_gridfile_dtor( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo::GridFile *file = (mongo::GridFile*)rsrc->ptr;
  if( file )
    delete file;
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mongo) {

  REGISTER_INI_ENTRIES();

  le_connection = zend_register_list_destructors_ex(php_connection_dtor, NULL, PHP_CONNECTION_RES_NAME, module_number);
  le_pconnection = zend_register_list_destructors_ex(NULL, php_connection_dtor, PHP_CONNECTION_RES_NAME, module_number);
  le_db_cursor = zend_register_list_destructors_ex(NULL, NULL, PHP_DB_CURSOR_RES_NAME, module_number);
  le_gridfs = zend_register_list_destructors_ex(php_gridfs_dtor, NULL, PHP_GRIDFS_RES_NAME, module_number);
  le_gridfile = zend_register_list_destructors_ex(php_gridfile_dtor, NULL, PHP_GRIDFILE_RES_NAME, module_number);

  zend_class_entry id; 
  INIT_CLASS_ENTRY(id, "MongoId", mongo_id_functions); 
  mongo_id_class = zend_register_internal_class(&id TSRMLS_CC); 

  zend_class_entry date; 
  INIT_CLASS_ENTRY(date, "MongoDate", mongo_date_functions); 
  mongo_date_class = zend_register_internal_class(&date TSRMLS_CC); 

  zend_class_entry regex; 
  INIT_CLASS_ENTRY(regex, "MongoRegex", mongo_regex_functions); 
  mongo_regex_class = zend_register_internal_class(&regex TSRMLS_CC); 

  zend_class_entry bindata; 
  INIT_CLASS_ENTRY(bindata, "MongoBinData", mongo_bindata_functions); 
  mongo_bindata_class = zend_register_internal_class(&bindata TSRMLS_CC); 

  return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */ 
PHP_MSHUTDOWN_FUNCTION(mongo) {
  UNREGISTER_INI_ENTRIES();

  return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION
 */ 
PHP_RINIT_FUNCTION(mongo) {
  MonGlo(num_links) = MonGlo(num_persistent); 
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mongo) {
  char buf[32]; 

  php_info_print_table_start();

  php_info_print_table_header(2, "MongoDB Support", "enabled");
  snprintf(buf, sizeof(buf), "%ld", MonGlo(num_persistent));
  php_info_print_table_row(2, "Active Persistent Connections", buf);
  snprintf(buf, sizeof(buf), "%ld", MonGlo(num_links));
  php_info_print_table_row(2, "Active Connections", buf);

  php_info_print_table_end(); 

  DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ mongo_connect
 */
PHP_FUNCTION(mongo_connect) {
  php_mongo_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */


/* {{{ mongo_pconnect
 */
PHP_FUNCTION(mongo_pconnect) {
  php_mongo_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */


/* {{{ proto bool mongo_close(resource connection) 
   Closes the database connection */
PHP_FUNCTION(mongo_close) {
  zval *zconn;
 
  if (ZEND_NUM_ARGS() != 1 ) {
     zend_error( E_WARNING, "expected 1 parameter, got %d parameters", ZEND_NUM_ARGS() );
     RETURN_FALSE;
  }
  else if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zconn) == FAILURE) {
     zend_error( E_WARNING, "incorrect parameter types, expected mongo_close( connection )" );
     RETURN_FALSE;
  }

  zend_list_delete(Z_LVAL_P(zconn));
  RETURN_TRUE;
}
/* }}} */


/* {{{ proto cursor mongo_query(resource connection, string ns, array query, int limit, int skip, array sort, array fields, array hint) 
   Query the database */
PHP_FUNCTION(mongo_query) {
  zval *zconn, *zquery, *zsort, *zfields, *zhint;
  char *collection;
  int limit, skip, collection_len;
  mongo::DBClientConnection *conn_ptr;
  std::auto_ptr<mongo::DBClientCursor> cursor;

  if( ZEND_NUM_ARGS() != 8 ) {
      zend_error( E_WARNING, "expected 8 parameters, got %d parameters", ZEND_NUM_ARGS() );
      RETURN_FALSE;
  }
  else if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsallaaa", &zconn, &collection, &collection_len, &zquery, &skip, &limit, &zsort, &zfields, &zhint) == FAILURE ) {
      zend_error( E_WARNING, "incorrect parameter types, expected mongo_query( connection, string, array, int, int, array, array, array )" );
      RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(conn_ptr, mongo::DBClientConnection*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo::BSONObjBuilder *bquery = new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  mongo::BSONObj query = bquery->done();
  mongo::Query q = mongo::Query( query );

  mongo::BSONObjBuilder *bfields = new mongo::BSONObjBuilder();
  int num_fields = php_array_to_bson( bfields, Z_ARRVAL_P( zfields ) );
  mongo::BSONObj fields = bfields->done();

  mongo::BSONObjBuilder *bhint = new mongo::BSONObjBuilder();
  int n = php_array_to_bson( bhint, Z_ARRVAL_P( zhint ) );
  if( n > 0 ) {
    mongo::BSONObj hint = bhint->done();
    q.hint( hint );
  }
  
  mongo::BSONObjBuilder *bsort = new mongo::BSONObjBuilder();
  n = php_array_to_bson( bsort, Z_ARRVAL_P( zsort ) );
  if( n > 0 ) {
    q.sort(bsort->done());
  }

  if (num_fields == 0) {
    cursor = conn_ptr->query( (const char*)collection, q, limit, skip );
  }
  else {
    cursor = conn_ptr->query( (const char*)collection, q, limit, skip, &fields );
  }

  mongo::DBClientCursor *c = cursor.release();

  delete bquery;
  delete bfields;
  delete bhint;
  delete bsort;
  // c has a registered dtor

  ZEND_REGISTER_RESOURCE( return_value, c, le_db_cursor );
}
/* }}} */


/* {{{ proto array mongo_find_one(resource connection, string ns, array query) 
   Query the database for one record */
PHP_FUNCTION(mongo_find_one) {
  zval *zconn, *zquery;
  char *collection;
  int collection_len;
  mongo::BSONObj query;
  mongo::DBClientConnection *conn_ptr;

  if( ZEND_NUM_ARGS() != 3 ) {
    zend_error( E_WARNING, "expected 3 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_find_one( connection, string, array )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(conn_ptr, mongo::DBClientConnection*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo::BSONObjBuilder *bquery = new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  query = bquery->done();

  mongo::BSONObj obj = conn_ptr->findOne( (const char*)collection, query );
  delete bquery;

  array_init(return_value);
  bson_to_php_array(&obj, return_value);
}
/* }}} */


/* {{{ proto bool mongo_remove(resource connection, string ns, array query) 
   Remove records from the database */
PHP_FUNCTION(mongo_remove) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;
  zend_bool justOne = 0;
  mongo::DBClientConnection *conn_ptr;

  if( ZEND_NUM_ARGS() != 4 ) {
    zend_error( E_WARNING, "expected 4 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsab", &zconn, &collection, &collection_len, &zarray, &justOne) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_remove( connection, string, array, bool )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(conn_ptr, mongo::DBClientConnection*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo::BSONObjBuilder *rarray = new mongo::BSONObjBuilder(); 
  php_array_to_bson( rarray, Z_ARRVAL_P(zarray) );
  conn_ptr->remove( collection, rarray->done(), justOne );

  delete rarray;
  RETURN_TRUE;
}
/* }}} */


/* {{{ proto bool mongo_insert(resource connection, string ns, array obj) 
   Insert a record to the database */
PHP_FUNCTION(mongo_insert) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;
  mongo::DBClientConnection *conn_ptr;

  if (ZEND_NUM_ARGS() != 3 ) {
    zend_error( E_WARNING, "expected 3 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_insert( connection, string, array )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(conn_ptr, mongo::DBClientConnection*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo::BSONObjBuilder *obj_builder = new mongo::BSONObjBuilder();
  HashTable *php_array = Z_ARRVAL_P(zarray);
  if( !zend_hash_exists( php_array, "_id", 3 ) )
      prep_obj_for_db( obj_builder );
  php_array_to_bson( obj_builder, php_array );

  conn_ptr->insert( collection, obj_builder->done() );

  delete obj_builder;
  RETURN_TRUE;
}
/* }}} */

PHP_FUNCTION(mongo_batch_insert) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;
  mongo::DBClientConnection *conn_ptr;

  if (ZEND_NUM_ARGS() != 3 ) {
    zend_error( E_WARNING, "expected 3 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_insert( connection, string, array )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(conn_ptr, mongo::DBClientConnection*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  vector<mongo::BSONObj> inserter;
  HashTable *php_array = Z_ARRVAL_P(zarray);
  HashPosition pointer;
  zval **data;
  for(zend_hash_internal_pointer_reset_ex(php_array, &pointer); 
      zend_hash_get_current_data_ex(php_array, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(php_array, &pointer)) {
    mongo::BSONObjBuilder *obj_builder = new mongo::BSONObjBuilder();
    HashTable *insert_elem = Z_ARRVAL_PP(data);

    php_array_to_bson(obj_builder, insert_elem);
    if (!zend_hash_exists(insert_elem, "_id", 3)) {
      prep_obj_for_db(obj_builder);
    }
    inserter.push_back(obj_builder->done());
  }

  conn_ptr->insert(collection, inserter);
  RETURN_TRUE;
}


/* {{{ proto bool mongo_update(resource connection, string ns, array query, array replacement, bool upsert) 
   Update a record in the database */
PHP_FUNCTION(mongo_update) {
  zval *zconn, *zquery, *zobj;
  char *collection;
  int collection_len;
  zend_bool zupsert = 0;
  mongo::DBClientConnection *conn_ptr;
  int num_args = ZEND_NUM_ARGS();
  if ( num_args != 5 ) {
    zend_error( E_WARNING, "expected 5 parameters, got %d parameters", num_args );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(num_args TSRMLS_CC, "rsaab", &zconn, &collection, &collection_len, &zquery, &zobj, &zupsert) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_update( connection, string, array, array, bool )");
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(conn_ptr, mongo::DBClientConnection*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo::BSONObjBuilder *bquery =  new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  mongo::BSONObjBuilder *bfields = new mongo::BSONObjBuilder();
  php_array_to_bson( bfields, Z_ARRVAL_P( zobj ) );
  conn_ptr->update( collection, bquery->done(), bfields->done(), (int)zupsert );

  delete bquery;
  delete bfields;
  RETURN_TRUE;
}
/* }}} */


/* {{{ proto bool mongo_has_next(resource cursor) 
   Check if a cursor has another record. */
PHP_FUNCTION( mongo_has_next ) {
  zval *zcursor;

  int argc = ZEND_NUM_ARGS();
  if (argc != 1 ) {
    zend_error( E_WARNING, "expected 1 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if( zend_parse_parameters(argc TSRMLS_CC, "r", &zcursor) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_has_next( cursor ), got %d parameters", argc );
    RETURN_FALSE;
  }

  mongo::DBClientCursor *c = (mongo::DBClientCursor*)zend_fetch_resource(&zcursor TSRMLS_CC, -1, PHP_DB_CURSOR_RES_NAME, NULL, 1, le_db_cursor);

  bool more = c->more();
  RETURN_BOOL(more);
}
/* }}} */


/* {{{ proto array mongo_next(resource cursor) 
   Get the next record from a cursor */
PHP_FUNCTION( mongo_next ) {
  zval *zcursor;
  int argc;
  mongo::BSONObj bson;
  mongo::DBClientCursor *c;

  argc = ZEND_NUM_ARGS();
  if (argc != 1 ) {
    zend_error( E_WARNING, "expected 1 parameter, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(argc TSRMLS_CC, "r", &zcursor) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter type, expected mongo_next( cursor )" );
    RETURN_FALSE;
  }

  c = (mongo::DBClientCursor*)zend_fetch_resource(&zcursor TSRMLS_CC, -1, PHP_DB_CURSOR_RES_NAME, NULL, 1, le_db_cursor);

  bson = c->next();

  array_init(return_value);
  bson_to_php_array(&bson, return_value);
}
/* }}} */


/* {{{ php_mongo_do_connect
 */
static void php_mongo_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent) {
  mongo::DBClientConnection *conn;
  char *server, *uname, *pass, *key;
  zend_bool auto_reconnect, lazy;
  int server_len, uname_len, pass_len, key_len;
  zend_rsrc_list_entry *le;
  string error;
  
  /* make sure that there aren't too many links already */
  if (MonGlo(max_links) > -1 &&
      MonGlo(max_links) <= MonGlo(num_links)) {
    RETURN_FALSE;
  }
  /* if persistent links aren't allowed, just create a normal link */
  if (!MonGlo(allow_persistent)) {
    persistent = 0;
  }
  /* make sure that there aren't too many persistent links already */
  if (persistent &&
      MonGlo(max_persistent) > -1 &&
      MonGlo(max_persistent) <= MonGlo(num_persistent)) {
    RETURN_FALSE;
  }

  int argc = ZEND_NUM_ARGS();
  if (persistent) {
    if (argc != 5) {
      zend_error( E_WARNING, "expected 5 parameters, got %d parameters", argc );
      RETURN_FALSE;
    }
    else if (zend_parse_parameters(argc TSRMLS_CC, "sssbb", &server, &server_len, &uname, &uname_len, &pass, &pass_len, &auto_reconnect, &lazy) == FAILURE) {
      zend_error( E_WARNING, "incorrect parameter types, expected: mongo_pconnect( string, string, string, bool, bool )" );
      RETURN_FALSE;
    }

    key_len = spprintf(&key, 0, "%s_%s_%s", server, uname, pass);
    /* if a connection is found, return it */
    if (zend_hash_find(&EG(persistent_list), key, key_len+1, (void**)&le) == SUCCESS) {
      conn = (mongo::DBClientConnection*)le->ptr;
      ZEND_REGISTER_RESOURCE(return_value, conn, le_pconnection);
      efree(key);
      return;
    }
    /* if lazy and no connection was found, return */
    else if(lazy) {
      efree(key);
      RETURN_NULL();
    }
    efree(key);
  }
  /* non-persistent */
  else {
    if( argc != 2 ) {
      zend_error( E_WARNING, "expected 2 parameters, got %d parameters", argc );
      RETURN_FALSE;
    }
    else if( zend_parse_parameters(argc TSRMLS_CC, "sb", &server, &server_len, &auto_reconnect) == FAILURE ) {
      zend_error( E_WARNING, "incorrect parameter types, expected: mongo_connect( string, bool )" );
      RETURN_FALSE;
    }
  }

  if ( server_len == 0 ) {
    zend_error( E_WARNING, "invalid host" );
    RETURN_FALSE;
  }

  conn = new mongo::DBClientConnection( (bool)auto_reconnect );
  if ( ! conn->connect( server, error ) ){
    zend_error( E_WARNING, "%s", error.c_str() );
    RETURN_FALSE;
  }

  // store a reference in the persistence list
  if (persistent) {
    zend_rsrc_list_entry new_le; 

    key_len = spprintf(&key, 0, "%s_%s_%s", server, uname, pass);
    Z_TYPE(new_le) = le_pconnection;
    new_le.ptr = conn;

    if (zend_hash_update(&EG(persistent_list), key, key_len+1, (void*)&new_le, sizeof(zend_rsrc_list_entry), NULL)==FAILURE) { 
      delete conn;
      efree(key);
      RETURN_FALSE;
    }
    efree(key);

    ZEND_REGISTER_RESOURCE(return_value, conn, le_pconnection);
    MonGlo(num_persistent)++;
  }
  // otherwise, just return the connection
  else {
    ZEND_REGISTER_RESOURCE(return_value, conn, le_connection);    
  }
  MonGlo(num_links)++;
}
/* }}} */
