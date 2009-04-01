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

#include "gridfs.h"
#include "mongo.h"
#include "mongo_types.h"
#include "bson.h"

extern zend_class_entry *mongo_id_class;

extern int le_connection;
extern int le_pconnection;
extern int le_db_cursor;
extern int le_gridfs;
extern int le_gridfile;

/* {{{ proto resource mongo_gridfs_init() 
   Creates a new gridfs connection point */
PHP_FUNCTION( mongo_gridfs_init ) {
  zval *zconn;
  char *dbname, *prefix;
  int dbname_len, prefix_len;
  mongo_link *link;

  if( ZEND_NUM_ARGS() != 3 ) {
    zend_error( E_WARNING, "expected 3 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zconn, &dbname, &dbname_len, &prefix, &prefix_len ) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_gridfs_init( connection, string, string )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo_gridfs *fs = (mongo_gridfs*)emalloc(sizeof(mongo_gridfs));
  fs->link = link;

  fs->db = (char*)emalloc(dbname_len);
  memcpy(fs->db, dbname, dbname_len);
  fs->db_len = dbname_len;

  fs->prefix = (char*)emalloc(prefix_len);
  memcpy(fs->prefix, prefix, prefix_len);

  // collections
  spprintf(&fs->file_ns, 0, "%s.%s.files", dbname, prefix);
  spprintf(&fs->chunk_ns, 0, "%s.%s.chunks", dbname, prefix);

  ZEND_REGISTER_RESOURCE(return_value, fs, le_gridfs);
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
  //php_array_to_bson(bquery, Z_ARRVAL_P(zquery) TSRMLS_CC);
  mongo::BSONObj query = bquery->done();

  std::auto_ptr<mongo::DBClientCursor> cursor = fs->list( query );
  mongo::DBClientCursor *c = cursor.get();

  delete bquery;
  ZEND_REGISTER_RESOURCE( return_value, c, le_db_cursor );
}
/* }}} */


/* {{{ proto array mongo_gridfs_store(resource gridfs, string filename) 
   Store a file to the database */
PHP_FUNCTION(mongo_gridfs_store) {
  zval *zfile, *zfs;
  char *filename;
  int filename_len;
  mongo_gridfs *gridfs;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zfs, &filename, &filename_len) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE(gridfs, mongo_gridfs*, &zfs, -1, PHP_GRIDFS_RES_NAME, le_gridfs);

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    zend_error(E_WARNING, "couldn't open file %s\n", filename);
    RETURN_FALSE;
  }

  // get size
  FILE *temp_fp = fp;
  fseek(temp_fp, 0L, SEEK_END);
  long size = ftell(temp_fp);
  if (size >= 0xffffffff) {
    zend_error(E_WARNING, "file %s is too large: %d bytes\n", filename, size);
    RETURN_FALSE;
  }

  // create an id for the file
  zval id;
  create_id(&id, 0 TSRMLS_CC);

  long pos = 0;
  int chunk_num = 0, chunk_size;
  
  // insert chunks
  while (pos < size) {
    chunk_size = size-pos >= DEFAULT_CHUNK_SIZE ? DEFAULT_CHUNK_SIZE : size-pos;
    char buf[chunk_size];
    fread(buf, chunk_size, 1, fp);

    // create chunk
    zval *zchunk;
    MAKE_STD_ZVAL(zchunk);
    array_init(zchunk);

    add_assoc_zval(zchunk, "files_id", &id);
    //add_assoc_long(zchunk, "n", chunk_num);
    //add_assoc_stringl(zchunk, "data", buf, chunk_size, NO_DUP);

    // insert chunk
    mongo_do_insert(gridfs->link, gridfs->chunk_ns, zchunk TSRMLS_CC);
    
    // increment counters
    pos += chunk_size;
    chunk_num++;
    efree(zchunk);
  }
  
  MAKE_STD_ZVAL(zfile);
  array_init(zfile);
  add_assoc_zval(zfile, "_id", &id);
  add_assoc_stringl(zfile, "filename", filename, filename_len, NO_DUP);
  add_assoc_long(zfile, "length", size);
  add_assoc_long(zfile, "chunkSize", DEFAULT_CHUNK_SIZE);

  // get md5
  zval *zmd5;
  MAKE_STD_ZVAL(zmd5);
  array_init(zmd5);
  add_assoc_zval(zmd5, "filemd5", &id);
  add_assoc_string(zmd5, "root", gridfs->prefix, NO_DUP);

  char *cmd;
  spprintf(&cmd, 0, "%s.$cmd", gridfs->db);

  zval *fields;
  MAKE_STD_ZVAL(fields);
  array_init(fields);
  mongo_cursor *cursor = mongo_do_query(gridfs->link, cmd, 0, -1, zmd5, fields TSRMLS_CC);
  if (!mongo_do_has_next(cursor TSRMLS_CC)) {
    zend_error(E_WARNING, "couldn't hash file %s\n", filename);
    RETURN_FALSE;
  }

  zval *hash = mongo_do_next(cursor TSRMLS_CC);
  add_assoc_zval(zfile, "md5", hash);

  // insert file
  mongo_do_insert(gridfs->link, gridfs->file_ns, zfile TSRMLS_CC);

  // cleanup
  efree(hash);
  efree(fields);
  efree(zmd5);
  //  efree(id);
  efree(zfile);
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
  //  php_array_to_bson( bquery, Z_ARRVAL_P( zquery ) TSRMLS_CC);
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
