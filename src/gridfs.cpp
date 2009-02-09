
#include <php.h>
#include <mongo/client/gridfs.h>
#include <string.h>

#include "mongo.h"
#include "bson.h"

extern zend_class_entry *mongo_id_class;

extern int le_db_client_connection;
extern int le_db_cursor;
extern int le_gridfs;
extern int le_gridfile;

PHP_FUNCTION( mongo_gridfs_init ) {
  zval *zconn;
  char *dbname, *prefix;
  int dbname_len, prefix_len;

  if( ZEND_NUM_ARGS() == 3 ) {
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zconn, &dbname, &dbname_len, &prefix, &prefix_len ) == FAILURE) {
      zend_error( E_WARNING, "parameter parse failure\n" );
      RETURN_FALSE;
    }
  }
  else  {
    zend_error( E_WARNING, "wrong number of args\n" );
    RETURN_FALSE;
  }

  mongo::DBClientConnection *conn_ptr = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_DB_CLIENT_CONNECTION_RES_NAME, NULL, 1, le_db_client_connection);
  if (!conn_ptr) {
    zend_error( E_WARNING, "no db connection\n" );
    RETURN_FALSE;
  }

  std::string *dbname_s = new std::string( dbname, dbname_len );
  std::string *prefix_s = new std::string( prefix, prefix_len );

  mongo::GridFS *gridfs = new mongo::GridFS( *conn_ptr, *dbname_s, *prefix_s );
  ZEND_REGISTER_RESOURCE( return_value, gridfs, le_gridfs );
}

PHP_FUNCTION( mongo_gridfs_list ) {
  mongo::GridFS *fs;
  zval *zfs, *zquery;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ra", &zfs, &zquery) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(fs, mongo::GridFS*, &zfs, -1, PHP_GRIDFS_RES_NAME, le_gridfs);

  mongo::BSONObjBuilder *bquery = new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  mongo::BSONObj query = bquery->done();

  std::auto_ptr<mongo::DBClientCursor> cursor = fs->list( query );
  mongo::DBClientCursor *c = cursor.get();
  ZEND_REGISTER_RESOURCE( return_value, c, le_db_cursor );
}

PHP_FUNCTION( mongo_gridfs_store ) {
  mongo::GridFS *fs;
  zval *zfs;
  char *filename;
  int filename_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zfs, &filename, &filename_len) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(fs, mongo::GridFS*, &zfs, -1, PHP_GRIDFS_RES_NAME, le_gridfs);

  std::string *f = new std::string( filename, filename_len );
  mongo::BSONElement elem = fs->storeFile( *f );
  zval *zoid;
  
  mongo::OID oid = elem.__oid();
  std::string str = oid.str();
  char *c = (char*)str.c_str();
  
  MAKE_STD_ZVAL(zoid);
  object_init_ex(zoid, mongo_id_class);
  add_property_stringl( zoid, "id", c, strlen( c ), 1 );
  RETURN_ZVAL( zoid, 0, 1 );
}

PHP_FUNCTION( mongo_gridfs_find ) {
  mongo::GridFS *fs;
  zval *zfs, *zquery;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ra", &zfs, &zquery) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(fs, mongo::GridFS*, &zfs, -1, PHP_GRIDFS_RES_NAME, le_gridfs);

  mongo::BSONObjBuilder *bquery = new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  mongo::BSONObj query = bquery->done();

  mongo::GridFile file = fs->findFile( query );
  mongo::GridFile *file_ptr( &file );
  ZEND_REGISTER_RESOURCE( return_value, &file_ptr, le_gridfile );
}

PHP_FUNCTION( mongo_gridfile_exists ) {
  mongo::GridFile *file;
  zval *zfile;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile ) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(file, mongo::GridFile*, &zfile, -1, PHP_GRIDFILE_RES_NAME, le_gridfile);

  bool exists = file->exists();
  RETURN_BOOL( exists ); 
}

PHP_FUNCTION( mongo_gridfile_filename ) {
  mongo::GridFile *file;
  zval *zfile;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile ) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(file, mongo::GridFile*, &zfile, -1, PHP_GRIDFILE_RES_NAME, le_gridfile);

  string name = file->getFilename();
  RETURN_STRING( (char*)name.c_str(), 1 ); 
}

PHP_FUNCTION( mongo_gridfile_size ) {
  mongo::GridFile *file;
  zval *zfile;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile ) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(file, mongo::GridFile*, &zfile, -1, PHP_GRIDFILE_RES_NAME, le_gridfile);

  php_printf("breaking now\n");
  long len = file->getContentLength();
  php_printf("broke\n");
  RETURN_LONG( len );
}

