// mongo.cpp

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <php.h>
#include <mongo/client/dbclient.h>
#include <mongo/client/gridfs.h>

#include "mongo.h"
#include "mongo_id.h"
#include "bson.h"
#include "gridfs.h"

zend_class_entry *mongo_id_class;

/** Resources */
int le_db_client_connection;
int le_db_cursor;
int le_gridfs;
int le_gridfile;

static function_entry mongo_functions[] = {
  PHP_FE( mongo_connect , NULL )
  PHP_FE( mongo_close , NULL )
  PHP_FE( mongo_remove , NULL )
  PHP_FE( mongo_query , NULL )
  PHP_FE( mongo_find_one , NULL )
  PHP_FE( mongo_insert , NULL )
  PHP_FE( mongo_update , NULL )
  PHP_FE( mongo_has_next , NULL )
  PHP_FE( mongo_next , NULL )
  PHP_FE( mongo_gridfs_init , NULL )
  PHP_FE( mongo_gridfs_list , NULL )
  PHP_FE( mongo_gridfs_store , NULL )
  PHP_FE( mongo_gridfs_find , NULL )
  PHP_FE( mongo_gridfile_exists , NULL )
  PHP_FE( mongo_gridfile_filename , NULL )
  PHP_FE( mongo_gridfile_size , NULL )
  {NULL, NULL, NULL}
};

static function_entry mongo_id_functions[] = {
  PHP_NAMED_FE( __construct, PHP_FN( mongo_id___construct ), NULL )
  PHP_NAMED_FE( __toString, PHP_FN( mongo_id___toString ), NULL )
  { NULL, NULL, NULL }
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
  le_db_cursor = zend_register_list_destructors_ex(NULL, NULL, PHP_DB_CURSOR_RES_NAME, module_number);
  le_gridfs = zend_register_list_destructors_ex(NULL, NULL, PHP_GRIDFS_RES_NAME, module_number);
  le_gridfile = zend_register_list_destructors_ex(NULL, NULL, PHP_GRIDFILE_RES_NAME, module_number);

  zend_class_entry id; 
  INIT_CLASS_ENTRY(id, "mongo_id", mongo_id_functions); 
  mongo_id_class = zend_register_internal_class(&id TSRMLS_CC); 

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
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(conn, mongo::DBClientConnection*, &zconn, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, le_db_client_connection);
  zval_dtor( zconn );
  RETURN_TRUE;
}

PHP_FUNCTION(mongo_query) {
  zval *zconn, *zquery, *zsort, *zfields, *zhint;
  char *collection;
  int limit, skip, collection_len;

  int argc = ZEND_NUM_ARGS();
  switch( argc ) {
  case 7:
    if (zend_parse_parameters(argc TSRMLS_CC, "rsalaaa", &zconn, &collection, &collection_len, &zquery, &skip, &zsort, &zhint ) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  case 8:
    if (zend_parse_parameters(argc TSRMLS_CC, "rsallaaa", &zconn, &collection, &collection_len, &zquery, &skip, &limit, &zsort, &zfields, &zhint) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  default:
    zend_error( E_WARNING, "wrong number of args\n" );
    RETURN_FALSE;
    break;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    zend_error( E_WARNING, "no db connection\n" );
    RETURN_FALSE;
  }

  mongo::BSONObjBuilder *bquery = new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  mongo::BSONObj query = bquery->done();
  mongo::Query *q = new mongo::Query( query );

  mongo::BSONObjBuilder *bfields = new mongo::BSONObjBuilder();
  int num_fields = php_array_to_bson( bfields, Z_ARRVAL_P( zfields ) );
  mongo::BSONObj fields = bfields->done();

  mongo::BSONObjBuilder *bhint = new mongo::BSONObjBuilder();
  php_array_to_bson( bhint, Z_ARRVAL_P( zhint ) );
  mongo::BSONObj hint = bhint->done();
  q->hint( hint );

  mongo::BSONObjBuilder *bsort = new mongo::BSONObjBuilder();
  php_array_to_bson( bsort, Z_ARRVAL_P( zsort ) );
  mongo::BSONObj sort = bsort->done();
  q->sort( sort );

  std::auto_ptr<mongo::DBClientCursor> cursor;
  if( num_fields == 0 )
    cursor = conn_ptr->query( (const char*)collection, *q, limit, skip );
  else
    cursor = conn_ptr->query( (const char*)collection, *q, limit, skip, &fields );

  mongo::DBClientCursor *c = cursor.get();
  ZEND_REGISTER_RESOURCE( return_value, c, le_db_cursor );
}


PHP_FUNCTION(mongo_find_one) {
  zval *zconn, *zquery;
  char *collection;
  int collection_len;
  mongo::BSONObj query;

  int num_args = ZEND_NUM_ARGS();
  switch( num_args ) {
  case 0:
  case 1:
  case 2:
    zend_error( E_WARNING, "too few args" );
    RETURN_FALSE;
    break;
  case 3:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zquery) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  default:
    zend_error( E_WARNING, "too many args\n" );
    RETURN_FALSE;
    break;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    zend_error( E_WARNING, "no db connection\n" );
    RETURN_FALSE;
  }

  mongo::BSONObjBuilder *bquery = new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  query = bquery->done();

  mongo::BSONObj obj = conn_ptr->findOne( (const char*)collection, query );
  zval *array = bson_to_php_array( obj );
  RETURN_ZVAL( array, 0, 1 );
}



PHP_FUNCTION(mongo_remove) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;
  zend_bool justOne = 0;

  if ( ZEND_NUM_ARGS() == 3 ) {
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure (3)" );
      RETURN_FALSE;
    }
  }
  else if( ZEND_NUM_ARGS() == 4 ) {
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsab", &zconn, &collection, &collection_len, &zarray, &justOne) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure (4)" );
      RETURN_FALSE;
    }
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    zend_error( E_WARNING, "no db connection\n" );
    RETURN_FALSE;
  }

  mongo::BSONObjBuilder *rarray = new mongo::BSONObjBuilder(); 
  php_array_to_bson( rarray, Z_ARRVAL_P(zarray) );
  conn_ptr->remove( collection, rarray->done(), justOne );
  RETURN_TRUE;
}

PHP_FUNCTION(mongo_insert) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
    zend_error( E_WARNING, "mongo_insert: parameter parse failure\n" );
    RETURN_FALSE;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    zend_error( E_WARNING, "no db connection\n" );
    RETURN_FALSE;
  }

  mongo::BSONObjBuilder *obj_builder = new mongo::BSONObjBuilder();
  HashTable *php_array = Z_ARRVAL_P(zarray);
  if( !zend_hash_exists( php_array, "_id", 3 ) )
      prep_obj_for_db( obj_builder );
  php_array_to_bson( obj_builder, php_array );
  conn_ptr->insert( collection, obj_builder->done() );
  RETURN_TRUE;
}

PHP_FUNCTION(mongo_update) {
  zval *zconn, *zquery, *zobj;
  char *collection;
  int collection_len;
  zend_bool zupsert = 0;

  int num_args = ZEND_NUM_ARGS();
  switch( num_args ) {
  case 0:
  case 1:
  case 2:
  case 3:
    zend_error( E_WARNING, "too few args\n" );
    RETURN_FALSE;
    break;
  case 4:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsaa", &zconn, &collection, &collection_len, &zquery, &zobj) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  case 5:
    if (zend_parse_parameters(num_args TSRMLS_CC, "rsaab", &zconn, &collection, &collection_len, &zquery, &zobj, &zupsert) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  default:
    zend_error( E_WARNING, "too many args\n" );
    RETURN_FALSE;
    break;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    zend_error( E_WARNING, "no db connection\n" );
    RETURN_FALSE;
  }

  mongo::BSONObjBuilder *bquery =  new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  mongo::BSONObjBuilder *bfields = new mongo::BSONObjBuilder();
  php_array_to_bson( bfields, Z_ARRVAL_P( zobj ) );
  conn_ptr->update( collection, bquery->done(), bfields->done(), (int)zupsert );
  RETURN_TRUE;
}

PHP_FUNCTION( mongo_has_next ) {
  zval *zcursor;

  int argc = ZEND_NUM_ARGS();
  switch( argc ) {
  case 0:
    zend_error( E_WARNING, "too few args" );
    RETURN_FALSE;
    break;
  case 1:
    if (zend_parse_parameters(argc TSRMLS_CC, "r", &zcursor) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  default:
    zend_error( E_WARNING, "too many args\n" );
    RETURN_FALSE;
    break;
  }

  mongo::DBClientCursor *c = (mongo::DBClientCursor*)zend_fetch_resource(&zcursor TSRMLS_CC, -1, PHP_DB_CURSOR_RES_NAME, NULL, 1, le_db_cursor);

  bool more = c->more();
  RETURN_BOOL(more);
}

PHP_FUNCTION( mongo_next ) {
  zval *zcursor;

  int argc = ZEND_NUM_ARGS();
  switch( argc ) {
  case 0:
    zend_error( E_WARNING, "too few args" );
    RETURN_FALSE;
    break;
  case 1:
    if (zend_parse_parameters(argc TSRMLS_CC, "r", &zcursor) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
    break;
  default:
    zend_error( E_WARNING, "too many args\n" );
    RETURN_FALSE;
    break;
  }

  mongo::DBClientCursor *c = (mongo::DBClientCursor*)zend_fetch_resource(&zcursor TSRMLS_CC, -1, PHP_DB_CURSOR_RES_NAME, NULL, 1, le_db_cursor);

  mongo::BSONObj bson = c->next();
  zval *array = bson_to_php_array( bson );
  RETURN_ZVAL( array, 0, 1 );
}

