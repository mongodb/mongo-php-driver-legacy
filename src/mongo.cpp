// mongo.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <php.h>
#include <mongo/client/dbclient.h>

#include "mongo.h"
#include "bson.h"

/** Resources */
int le_db_client_connection;

static function_entry mongo_functions[] = {
  PHP_FE( mongo_connect , NULL )
  PHP_FE( mongo_close , NULL )
  PHP_FE( mongo_remove , NULL )
  PHP_FE( mongo_query , NULL )
  PHP_FE( mongo_insert , NULL )
  PHP_FE( mongo_update , NULL )
  {NULL, NULL, NULL}
};

zend_module_entry mongo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  PHP_MONGO_EXTNAME,
  mongo_functions,
  PHP_MINIT(mongo),
  NULL,
  NULL,
  NULL,
  NULL,
#if ZEND_MODULE_API_NO >= 20010901
  PHP_MONGO_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_MONGO
ZEND_GET_MODULE(mongo)
#endif


PHP_MINIT_FUNCTION(mongo) {
  le_db_client_connection = zend_register_list_destructors_ex(NULL, NULL, PHP_DB_CLIENT_CONNECTION_RES_NAME, module_number);
  return SUCCESS;
}

PHP_FUNCTION(mongo_connect) {
  mongo::DBClientConnection *conn = new mongo::DBClientConnection( 1, 0 );
  char *server;
  int server_len;
  string error;
  
  switch( ZEND_NUM_ARGS() ) {
  case 0:
    server = "127.0.0.1";
    break;
  case 1:
    // get the server name from the first parameter
    if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &server, &server_len) == FAILURE ) {
      RETURN_FALSE;
    }
    break;
  default:
    RETURN_FALSE;
  }

  if ( server_len == 0 ) {
    RETURN_FALSE;
  }

  if ( ! conn->connect( server, error ) ){
    zend_error( E_WARNING, "%s", error.c_str() );
    RETURN_FALSE;
  }
  
  // return connection
  ZEND_REGISTER_RESOURCE( return_value, conn, le_db_client_connection );
}

PHP_FUNCTION(mongo_close) {
  mongo::DBClientConnection *conn;
  zval *zconn;
 
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zconn) == FAILURE) {
    RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(conn, mongo::DBClientConnection*, &zconn, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, le_db_client_connection);
  zval_dtor( zconn );
  RETURN_TRUE;
}

PHP_FUNCTION(mongo_query) {
  zval *zconn, *zquery, *zfields;
  char *collection;
  int limit, skip, collection_len, opts;

  int num_args = ZEND_NUM_ARGS();
  switch( num_args ) {
  case 0:
  case 1:
  // TODO: add find( db, ns ) (find all)
  case 2:
    php_printf( "mongo_find: too few args\n" );
    RETURN_FALSE;
    break;
  case 3:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery) == FAILURE) {
      php_printf( "mongo_find: parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  case 4:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery, &limit) == FAILURE) {
      php_printf( "mongo_find: parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  case 5:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery, &limit, &skip) == FAILURE) {
      php_printf( "mongo_find: parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  case 6:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery, &limit, &skip, &zfields) == FAILURE) {
      php_printf( "mongo_find: parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  case 7:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery, &limit, &skip, &zfields, &opts) == FAILURE) {
      php_printf( "mongo_find: parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  default:
    php_printf( "mongo_find: too many args\n" );
    RETURN_FALSE;
    break;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    php_printf( "mongo_find: no db connection\n" );
    RETURN_FALSE;
  }

  /*mongo::BSONObj bquery = php_array_to_bson( Z_ARRVAL_P( zquery ) );
  if( zfields )
    mongo::BSONObj bfields = php_array_to_bson( Z_ARRVAL_P( zfields ) );
  
    conn_ptr->query( collection, bquery, limit, skip, new mongo::BSONObj(), opts);*/
  RETURN_TRUE;
}

PHP_FUNCTION(mongo_remove) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;
  zend_bool justOne = 0;

  if ( ZEND_NUM_ARGS() == 3 ) {
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
      php_printf( "mongo_remove: parameter parse failure (3)\n" );
      RETURN_FALSE;
    }
  }
  else if( ZEND_NUM_ARGS() == 4 ) {
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsab", &zconn, &collection, &collection_len, &zarray, &justOne) == FAILURE) {
      php_printf( "mongo_remove: parameter parse failure (4)\n" );
      RETURN_FALSE;
    }
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    RETURN_FALSE;
  }

  /*  mongo::BSONObj rarray = php_array_to_bson( Z_ARRVAL_P(zarray) );
  conn_ptr->remove( collection, rarray, justOne );
  RETURN_TRUE;*/
}

PHP_FUNCTION(mongo_insert) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
    php_printf( "mongo_insert: parameter parse failure\n" );
    RETURN_FALSE;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    RETURN_FALSE;
  }

  mongo::BSONObjBuilder *obj_builder = new mongo::BSONObjBuilder();
  HashTable *php_array = Z_ARRVAL_P(zarray);
  if( !zend_hash_exists( php_array, "_id", 3 ) )
      prep_obj_for_db( obj_builder );
  php_array_to_bson( obj_builder, php_array );
  conn_ptr->insert( collection, obj_builder->doneAndDecouple() );
  php_printf("kisses! xoxoxoxo\n");
}

PHP_FUNCTION(mongo_update) {
  zval *zconn, *zquery, *zobj;
  char *collection;
  int collection_len;
  zend_bool upsert = 0;

  int num_args = ZEND_NUM_ARGS();
  switch( num_args ) {
  case 0:
  case 1:
  case 2:
  case 3:
    php_printf( "mongo_update: too few args\n" );
    RETURN_FALSE;
    break;
  case 4:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery, &zobj) == FAILURE) {
      php_printf( "mongo_update: parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  case 5:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery, &zobj, &upsert) == FAILURE) {
      php_printf( "mongo_update: parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  default:
    php_printf( "mongo_update: too many args\n" );
    RETURN_FALSE;
    break;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    RETURN_FALSE;
  }
  /*
  mongo::BSONObj bquery = php_array_to_bson( Z_ARRVAL_P( zquery ) );
  mongo::BSONObj bobj = php_array_to_bson( Z_ARRVAL_P( zobj ) );
  conn_ptr->update( collection, bquery, bobj, upsert );
  RETURN_TRUE;*/
}


