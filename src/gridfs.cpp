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

#include <php.h>
#include <mongo/client/gridfs.h>
#include <string.h>

#include "mongo.h"
#include "mongo_id.h"
#include "bson.h"

extern zend_class_entry *mongo_id_class;

extern int le_db_client_connection;
extern int le_db_cursor;
extern int le_gridfs;
extern int le_gridfile;

/* {{{ proto resource mongo_gridfs_init() 
   Creates a new gridfs connection point */
PHP_FUNCTION( mongo_gridfs_init ) {
  zval *zconn;
  char *dbname, *prefix;
  int dbname_len, prefix_len;

  if( ZEND_NUM_ARGS() != 3 ) {
    zend_error( E_WARNING, "expected 3 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zconn, &dbname, &dbname_len, &prefix, &prefix_len ) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_gridfs_init( connection, string, string )" );
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

  delete dbname_s;
  delete prefix_s;
  ZEND_REGISTER_RESOURCE( return_value, gridfs, le_gridfs );
}
/* }}} */


/* {{{ proto array mongo_gridfs_list(resource gridfs, array query) 
   List files in the database */
PHP_FUNCTION( mongo_gridfs_list ) {
  mongo::GridFS *fs;
  zval *zfs, *zquery;

  if( ZEND_NUM_ARGS() != 2 ) {
    zend_error( E_WARNING, "expected 2 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ra", &zfs, &zquery) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_gridfs_list( gridfs, array )" );
    RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(fs, mongo::GridFS*, &zfs, -1, PHP_GRIDFS_RES_NAME, le_gridfs);

  mongo::BSONObjBuilder *bquery = new mongo::BSONObjBuilder();
  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) );
  mongo::BSONObj query = bquery->done();

  std::auto_ptr<mongo::DBClientCursor> cursor = fs->list( query );
  mongo::DBClientCursor *c = cursor.get();

  delete bquery;
  ZEND_REGISTER_RESOURCE( return_value, c, le_db_cursor );
}
/* }}} */


/* {{{ proto array mongo_gridfs_store(resource gridfs, string filename) 
   Store a file to the database */
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
  mongo::BSONObj obj = fs->storeFile( *f );
  mongo::BSONElement elem = obj.findElement( "_id" );

  delete f;
  zval *ret = oid_to_mongo_id( elem.__oid() );
  RETURN_ZVAL( ret, 0, 1 );
}
/* }}} */


/* {{{ proto resource mongo_gridfs_find(resource gridfs, array query) 
   Retreive a file from the database */
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
  if (!file.exists()) {
    RETURN_NULL();
  }
  mongo::GridFile *file_ptr = new mongo::GridFile( file );

  delete bquery;
  ZEND_REGISTER_RESOURCE( return_value, file_ptr, le_gridfile );
}
/* }}} */

/* {{{ proto string mongo_gridfile_filename(resource gridfile) 
   Get a gridfile's filename */
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
/* }}} */


/* {{{ proto int mongo_gridfile_size(resource gridfile) 
   Get a gridfile's size */
PHP_FUNCTION( mongo_gridfile_size ) {
  mongo::GridFile *file;
  zval *zfile;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile ) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(file, mongo::GridFile*, &zfile, -1, PHP_GRIDFILE_RES_NAME, le_gridfile);

  long len = file->getContentLength();
  RETURN_LONG( len );
}
/* }}} */


/* {{{ proto int mongo_gridfile_write(resource gridfile) 
   Write a gridfile to the filesystem */
PHP_FUNCTION( mongo_gridfile_write ) {
  mongo::GridFile *file;
  char *filename;
  int filename_len;
  zval *zfile;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zfile, &filename, &filename_len ) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(file, mongo::GridFile*, &zfile, -1, PHP_GRIDFILE_RES_NAME, le_gridfile);

  std::string *f = new std::string( filename, filename_len );
  long len = file->write( *f );
  delete f;
  RETURN_LONG( len );
}
/* }}} */
