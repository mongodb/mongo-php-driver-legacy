// mongo.c
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

#include <php.h>
#include <php_ini.h>

#include "mongo.h"
#include "mongo_types.h"
#include "bson.h"

/** Classes */
zend_class_entry *mongo_id_class, 
  *mongo_code_class, 
  *mongo_date_class, 
  *mongo_regex_class, 
  *mongo_bindata_class;

/** Resources */
int le_connection, le_pconnection, le_db_cursor, le_gridfs, le_gridfile;

ZEND_DECLARE_MODULE_GLOBALS(mongo)
static PHP_GINIT_FUNCTION(mongo);

static function_entry mongo_functions[] = {
  PHP_FE( mongo_connect , NULL )
  PHP_FE( mongo_close , NULL )
  PHP_FE( mongo_remove , NULL )
  PHP_FE( mongo_query , NULL )
  PHP_FE( mongo_insert , NULL )
  PHP_FE( mongo_batch_insert , NULL )
  PHP_FE( mongo_update , NULL )
  PHP_FE( mongo_has_next , NULL )
  PHP_FE( mongo_next , NULL )
  PHP_FE( mongo_gridfs_init , NULL )
  PHP_FE( mongo_gridfs_store , NULL )
  PHP_FE( mongo_gridfile_write , NULL )
  {NULL, NULL, NULL}
};

static function_entry mongo_id_functions[] = {
  PHP_NAMED_FE( __construct, PHP_FN( mongo_id___construct ), NULL )
  PHP_NAMED_FE( __toString, PHP_FN( mongo_id___toString ), NULL )
  { NULL, NULL, NULL }
};


static function_entry mongo_code_functions[] = {
  PHP_NAMED_FE( __construct, PHP_FN( mongo_code___construct ), NULL )
  PHP_NAMED_FE( __toString, PHP_FN( mongo_code___toString ), NULL )
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
STD_PHP_INI_ENTRY("mongo.chunk_size", "262144", PHP_INI_ALL, OnUpdateLong, default_port, zend_mongo_globals, mongo_globals) 
PHP_INI_END()
/* }}} */


/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mongo){
  mongo_globals->num_persistent = 0; 
  mongo_globals->num_links = 0; 
  mongo_globals->auto_reconnect = 0; 
  mongo_globals->default_host = "localhost"; 
  mongo_globals->default_port = 27017; 
  mongo_globals->request_id = 3;
  mongo_globals->chunk_size = 262144;
}
/* }}} */

static void php_connection_dtor( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo_link *conn = (mongo_link*)rsrc->ptr;
  if (conn) {
    // close the connection
    close(conn->socket);

    // free strings
    if (conn->host) {
      efree(conn->host);
    }
    if (conn->username) {
      efree(conn->username);
    }
    if (conn->password) {
      efree(conn->password);
    }

    // free connection
    efree(conn);

    // if it's a persistent connection, decrement the 
    // number of open persistent links
    if (rsrc->type == le_pconnection) {
      MonGlo(num_persistent)--;
    }
    // decrement the total number of links
    MonGlo(num_links)--;
  }
}

static void php_gridfs_dtor( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo_gridfs *fs = (mongo_gridfs*)rsrc->ptr;
  if (fs) {
    if (fs->db) {
      efree(fs->db);
    }
    if (fs->file_ns) {
      efree(fs->file_ns);
    }
    if (fs->chunk_ns) {
      efree(fs->chunk_ns);
    }

    efree(fs);
  }
}


// tell db to destroy its cursor
static void kill_cursor(mongo_cursor *cursor TSRMLS_DC) {
  unsigned char quickbuf[128];
  buffer buf;
  buf.pos = quickbuf;
  buf.start = buf.pos;
  buf.end = buf.start + 128;

  // std header
  CREATE_MSG_HEADER(0, MonGlo(request_id)++, OP_KILL_CURSORS);
  APPEND_HEADER(buf);
  // 0 - reserved
  serialize_int(&buf, 0);
  // # of cursors
  serialize_int(&buf, 1);
  // cursor ids
  serialize_long(&buf, cursor->cursor_id);
  serialize_size(buf.start, &buf);

  say(&cursor->link, &buf TSRMLS_CC);
}

static void free_cursor(mongo_cursor *cursor) {
  // free mem
  if (cursor->buf_start) {
    efree(cursor->buf_start);
  }
  efree(cursor);
}

static void php_cursor_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
  mongo_cursor *cursor = (mongo_cursor*)rsrc->ptr;
  if (cursor) {
    kill_cursor(cursor TSRMLS_CC);
    free_cursor(cursor);
  }
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mongo) {

  REGISTER_INI_ENTRIES();

  le_connection = zend_register_list_destructors_ex(php_connection_dtor, NULL, PHP_CONNECTION_RES_NAME, module_number);
  le_pconnection = zend_register_list_destructors_ex(NULL, php_connection_dtor, PHP_CONNECTION_RES_NAME, module_number);
  le_db_cursor = zend_register_list_destructors_ex(php_cursor_dtor, NULL, PHP_DB_CURSOR_RES_NAME, module_number);
  le_gridfs = zend_register_list_destructors_ex(php_gridfs_dtor, NULL, PHP_GRIDFS_RES_NAME, module_number);

  zend_class_entry bindata; 
  INIT_CLASS_ENTRY(bindata, "MongoBinData", mongo_bindata_functions); 
  mongo_bindata_class = zend_register_internal_class(&bindata TSRMLS_CC); 

  zend_class_entry code; 
  INIT_CLASS_ENTRY(code, "MongoCode", mongo_code_functions); 
  mongo_code_class = zend_register_internal_class(&code TSRMLS_CC); 

  zend_class_entry date; 
  INIT_CLASS_ENTRY(date, "MongoDate", mongo_date_functions); 
  mongo_date_class = zend_register_internal_class(&date TSRMLS_CC); 

  zend_class_entry id; 
  INIT_CLASS_ENTRY(id, "MongoId", mongo_id_functions); 
  mongo_id_class = zend_register_internal_class(&id TSRMLS_CC); 

  zend_class_entry regex; 
  INIT_CLASS_ENTRY(regex, "MongoRegex", mongo_regex_functions); 
  mongo_regex_class = zend_register_internal_class(&regex TSRMLS_CC); 


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
  php_mongo_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */


/* {{{ mongo_close() 
 */
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


/* {{{ mongo_query() 
 */
PHP_FUNCTION(mongo_query) {
  mongo_link *link;
  zval *zconn, *zquery, *zfields;
  char *collection;
  int limit, skip, collection_len, argc = ZEND_NUM_ARGS();

  if (argc != 6) {
    zend_error( E_WARNING, "expected 6 parameters, got %d parameters", argc);
    RETURN_FALSE;
  }
  else if (zend_parse_parameters(argc TSRMLS_CC, "rsalla", &zconn, &collection, &collection_len, &zquery, &skip, &limit, &zfields) == FAILURE) {
    zend_error(E_WARNING, "incorrect parameter types, expected mongo_query(connection, string, array, int, int, array)");
    RETURN_FALSE;
  }
  
  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo_cursor *cursor = mongo_do_query(link, collection, skip, limit, zquery, zfields TSRMLS_CC);
  ZEND_REGISTER_RESOURCE(return_value, cursor, le_db_cursor);
}
/* }}} */


/* {{{ mongo_remove
 */
PHP_FUNCTION(mongo_remove) {
  zval *zconn, *zarray;
  char *collection, *start;
  int collection_len, mflags = 0;
  zend_bool justOne = 0;
  mongo_link *link;

  if( ZEND_NUM_ARGS() != 4 ) {
    zend_error( E_WARNING, "expected 4 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsab", &zconn, &collection, &collection_len, &zarray, &justOne) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_remove( connection, string, array, bool )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  CREATE_BUF(buf, INITIAL_BUF_SIZE);

  HashTable *array = Z_ARRVAL_P(zarray);
  CREATE_HEADER(buf, collection, collection_len, OP_DELETE);

  if (justOne || zend_hash_find(array, "_id", 4, NULL) == SUCCESS) {
    mflags |= 1;
  }

  serialize_int(&buf, mflags);
  zval_to_bson(&buf, array TSRMLS_CC);
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(say(link, &buf TSRMLS_CC)+1);
  efree(buf.start);
}
/* }}} */


/* {{{ mongo_insert
 */
PHP_FUNCTION(mongo_insert) {
  zval *zconn, *zarray;
  char *collection;
  int collection_len;
  mongo_link *link;

  if (ZEND_NUM_ARGS() != 3 ) {
    zend_error( E_WARNING, "expected 3 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_insert( connection, string, array )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  int response = mongo_do_insert(link, collection, zarray TSRMLS_CC);
  RETURN_BOOL(response == SUCCESS);
}
/* }}} */


PHP_FUNCTION(mongo_batch_insert) {
  mongo_link *link;
  HashPosition pointer;
  zval *zconn, *zarray, **data;
  char *collection;
  int collection_len;

  if (ZEND_NUM_ARGS() != 3 ) {
    zend_error( E_WARNING, "expected 3 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsa", &zconn, &collection, &collection_len, &zarray) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_batch_insert( connection, string, array )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, collection, collection_len, OP_INSERT);

  HashTable *php_array = Z_ARRVAL_P(zarray);
  for(zend_hash_internal_pointer_reset_ex(php_array, &pointer); 
      zend_hash_get_current_data_ex(php_array, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(php_array, &pointer)) {

    unsigned char *istart = buf.pos;
    zval_to_bson(&buf, Z_ARRVAL_PP(data) TSRMLS_CC);
    serialize_size(istart, &buf);
  }
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(say(link, &buf TSRMLS_CC)+1);
  efree(buf.start);
}


/* {{{ mongo_update
 */
PHP_FUNCTION(mongo_update) {
  zval *zconn, *zquery, *zobj;
  char *collection;
  int collection_len;
  zend_bool zupsert = 0;
  mongo_link *link;
  int argc = ZEND_NUM_ARGS();

  if ( argc != 5 ) {
    zend_error( E_WARNING, "expected 5 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(argc TSRMLS_CC, "rsaab", &zconn, &collection, &collection_len, &zquery, &zobj, &zupsert) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_update( connection, string, array, array, bool )");
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, collection, collection_len, OP_UPDATE);
  serialize_int(&buf, zupsert);
  zval_to_bson(&buf, Z_ARRVAL_P(zquery) TSRMLS_CC);
  zval_to_bson(&buf, Z_ARRVAL_P(zobj) TSRMLS_CC);
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(say(link, &buf TSRMLS_CC)+1);
  efree(buf.start);
}
/* }}} */


/* {{{ mongo_has_next 
 */
PHP_FUNCTION( mongo_has_next ) {
  zval *zcursor;
  mongo_cursor *cursor;
  int argc = ZEND_NUM_ARGS();

  if (argc != 1 ) {
    zend_error( E_WARNING, "expected 1 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if( zend_parse_parameters(argc TSRMLS_CC, "r", &zcursor) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_has_next( cursor ), got %d parameters", argc );
    RETURN_FALSE;
  }

  cursor = (mongo_cursor*)zend_fetch_resource(&zcursor TSRMLS_CC, -1, PHP_DB_CURSOR_RES_NAME, NULL, 1, le_db_cursor);
  RETURN_BOOL(mongo_do_has_next(cursor TSRMLS_CC));
}
/* }}} */


/* {{{ mongo_next
 */
PHP_FUNCTION(mongo_next) {
  zval *zcursor;
  int argc;
  mongo_cursor *cursor;

  argc = ZEND_NUM_ARGS();
  if (argc != 1 ) {
    zend_error( E_WARNING, "expected 1 parameter, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if(zend_parse_parameters(argc TSRMLS_CC, "r", &zcursor) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter type, expected mongo_next( cursor )" );
    RETURN_FALSE;
  }

  cursor = (mongo_cursor*)zend_fetch_resource(&zcursor TSRMLS_CC, -1, PHP_DB_CURSOR_RES_NAME, NULL, 1, le_db_cursor);
  zval *z = mongo_do_next(cursor TSRMLS_CC);
  RETURN_ZVAL(z, 0, 1);
}
/* }}} */



/* {{{ mongo_gridfs_init
 */
PHP_FUNCTION( mongo_gridfs_init ) {
  zval *zconn;
  char *dbname, *files, *chunks;
  int dbname_len, files_len, chunks_len;
  mongo_link *link;

  if( ZEND_NUM_ARGS() != 4 ) {
    zend_error( E_WARNING, "expected 4 parameters, got %d parameters", ZEND_NUM_ARGS() );
    RETURN_FALSE;
  }
  else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsss", &zconn, &dbname, &dbname_len, &files, &files_len, &chunks, &chunks_len) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_gridfs_init( connection, string, string )" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE2(link, mongo_link*, &zconn, -1, PHP_CONNECTION_RES_NAME, le_connection, le_pconnection); 

  mongo_gridfs *fs = (mongo_gridfs*)emalloc(sizeof(mongo_gridfs));
  fs->link = link;

  fs->db = (char*)emalloc(dbname_len);
  memcpy(fs->db, dbname, dbname_len);
  fs->db_len = dbname_len;

  // collections
  spprintf(&fs->file_ns, 0, "%s.%s", dbname, files);
  spprintf(&fs->chunk_ns, 0, "%s.%s", dbname, chunks);

  ZEND_REGISTER_RESOURCE(return_value, fs, le_gridfs);
}
/* }}} */


/* {{{ mongo_gridfs_store
 */
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
  fseek(fp, 0L, SEEK_END);
  long size = ftell(fp);
  if (size >= 0xffffffff) {
    zend_error(E_WARNING, "file %s is too large: %d bytes\n", filename, size);
    RETURN_FALSE;
  }


  // create an id for the file
  char cid[12];
  generate_id(cid);

  ALLOC_INIT_ZVAL(zfile);
  array_init(zfile);
  
  long pos = 0;
  int chunk_num = 0, chunk_size;

  fseek(fp, 0, SEEK_SET);
  // insert chunks
  while (pos < size) {
    chunk_size = size-pos >= DEFAULT_CHUNK_SIZE ? DEFAULT_CHUNK_SIZE : size-pos;
    char buf[chunk_size];
    fread(buf, 1, chunk_size, fp);

    zval *id;
    ALLOC_INIT_ZVAL(id);
    create_id(id, cid TSRMLS_CC);

    // create chunk
    zval *zchunk;
    ALLOC_INIT_ZVAL(zchunk);
    array_init(zchunk);

    add_assoc_zval(zchunk, "files_id", id);
    add_assoc_long(zchunk, "n", chunk_num);
    add_assoc_stringl(zchunk, "data", buf, chunk_size, DUP);

    // insert chunk
    mongo_do_insert(gridfs->link, gridfs->chunk_ns, zchunk TSRMLS_CC);
    
    // increment counters
    id->refcount++;
    pos += chunk_size;
    chunk_num++;
    zval_ptr_dtor(&id);
    zval_ptr_dtor(&zchunk);
  }
  fclose(fp);

  zval *id;
  ALLOC_INIT_ZVAL(id);
  create_id(id, cid TSRMLS_CC);
  add_assoc_zval(zfile, "_id", id);
  add_assoc_stringl(zfile, "filename", filename, filename_len, DUP);
  add_assoc_long(zfile, "length", size);
  add_assoc_long(zfile, "chunkSize", DEFAULT_CHUNK_SIZE);

  // get md5
  /*  zval *zmd5;
  ALLOC_INIT_ZVAL(zmd5);
  array_init(zmd5);

  add_assoc_zval(zmd5, "filemd5", id);
  add_assoc_string(zmd5, "root", gridfs->file_ns, DUP);

  char *cmd;
  spprintf(&cmd, 0, "%s.$cmd", gridfs->db);

  mongo_cursor *cursor = mongo_do_query(gridfs->link, cmd, 0, -1, zmd5, 0 TSRMLS_CC);

  zval_ptr_dtor(&zmd5);
  efree(cmd);
  if (!mongo_do_has_next(cursor TSRMLS_CC)) {
    zend_error(E_WARNING, "couldn't hash file %s\n", filename);
    RETURN_FALSE;
  }

  zval *hash = mongo_do_next(cursor TSRMLS_CC);
  add_assoc_zval(zfile, "md5", hash);
  */

  // insert file
  mongo_do_insert(gridfs->link, gridfs->file_ns, zfile TSRMLS_CC);
  //  zval_ptr_dtor(&hash);
  //  free_cursor(cursor);

  // cleanup
  zval_ptr_dtor(&id);
  zval_ptr_dtor(&zfile);
}
/* }}} */


/* {{{ mongo_gridfile_write
 */
PHP_FUNCTION(mongo_gridfile_write) {
  mongo_gridfs *fs;
  zval *zfs, *zquery;
  char *filename;
  int filename_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ras", &zfs, &zquery, &filename, &filename_len ) == FAILURE) {
     zend_error( E_WARNING, "parameter parse failure\n" );
     RETURN_FALSE;
  }
  ZEND_FETCH_RESOURCE(fs, mongo_gridfs*, &zfs, -1, PHP_GRIDFS_RES_NAME, le_gridfs);

  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    zend_error(E_WARNING, "couldn't open destination file %s", filename);
    RETURN_FALSE;
  }

  mongo_cursor *c = mongo_do_query(fs->link, fs->chunk_ns, 0, 0, zquery, 0 TSRMLS_CC);

  int total = 0;
  while (mongo_do_has_next(c TSRMLS_CC)) {
    zval *elem = mongo_do_next(c TSRMLS_CC);
    zval *zdata;
    HashTable *response = Z_ARRVAL_P(elem);
    if (zend_hash_find(response, "data", 5, (void**)&zdata) == FAILURE) {
      zval_ptr_dtor(&elem);
      zend_error(E_WARNING, "error reading chunk of file");
      RETURN_FALSE;
    }
    char *data = Z_STRVAL_P(zdata);
    int len = Z_STRLEN_P(zdata);

    int written = fwrite(data, 1, len, fp);
    if (written != len) {
      zend_error(E_WARNING, "incorrect byte count.  expected: %d, got %d", len, written);
    }
    total += written;

    zval_ptr_dtor(&elem);
  }

  free_cursor(c);
  fclose(fp);
  RETURN_LONG(total);
}
/* }}} */


mongo_cursor* mongo_do_query(mongo_link *link, char *collection, int skip, int limit, zval *zquery, zval *zfields TSRMLS_DC) {
  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, collection, strlen(collection), OP_QUERY);

  serialize_int(&buf, skip);
  serialize_int(&buf, limit);

  zval_to_bson(&buf, Z_ARRVAL_P(zquery) TSRMLS_CC);
  if (zfields && zend_hash_num_elements(Z_ARRVAL_P(zfields)) > 0) {
    zval_to_bson(&buf, Z_ARRVAL_P(zfields) TSRMLS_CC);
  }

  serialize_size(buf.start, &buf);

  // sends
  int sent = say(link, &buf TSRMLS_CC);
  efree(buf.start);
  if (sent == FAILURE) {
    return 0;
  }

  mongo_cursor *cursor = (mongo_cursor*)emalloc(sizeof(mongo_cursor));
  cursor->buf_start = 0;
  GET_RESPONSE_NS(link, cursor, collection, strlen(collection));
  cursor->limit = limit;
  return cursor;
}


int mongo_do_has_next(mongo_cursor *cursor TSRMLS_DC) {
  if (cursor->at < cursor->num) {
    return 1;
  }

  CREATE_BUF(buf, 256);
  CREATE_RESPONSE_HEADER(buf, cursor->ns, cursor->ns_len, cursor->header.request_id, OP_REPLY);
  serialize_int(&buf, cursor->limit);
  serialize_long(&buf, cursor->cursor_id);
  serialize_size(buf.start, &buf);

  // fails if we're out of elems
  if(say(&cursor->link, &buf TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return 0;
  }

  efree(buf.start);
  GET_RESPONSE(&cursor->link, cursor);

  return cursor->num > 0;
}


zval* mongo_do_next(mongo_cursor *cursor TSRMLS_DC) {
  if (cursor->at >= cursor->num) {
    GET_RESPONSE(&cursor->link, cursor);
  }

  if (cursor->at < cursor->num) {
    zval *elem;
    ALLOC_INIT_ZVAL(elem);
    array_init(elem);
    cursor->buf = bson_to_zval(cursor->buf, elem TSRMLS_CC);

    // increment cursor position
    cursor->at++;
    return elem;
  }
  return 0;
}

int mongo_do_insert(mongo_link *link, char *collection, zval *zarray TSRMLS_DC) {
  CREATE_BUF(buf, INITIAL_BUF_SIZE);
  CREATE_HEADER(buf, collection, strlen(collection), OP_INSERT);

  // adds data
  HashTable *obj = Z_ARRVAL_P(zarray);
  //  buf = prep_obj_for_db(buf, end, obj TSRMLS_CC);
  zval_to_bson(&buf, obj TSRMLS_CC);

  serialize_size(buf.start, &buf);
  
  // sends
  int response = say(link, &buf TSRMLS_CC);
  efree(buf.start);
  return response;
}


static int get_reply(mongo_link *link, mongo_cursor *cursor) {
  if(cursor->buf_start) {
    efree(cursor->buf_start);
  }

  if (recv(link->socket, &cursor->header.length, INT_32, FLAGS) == -1) {
    perror("recv");
    return FAILURE;
  }

  // make sure we're not getting crazy data
  if (cursor->header.length > MAX_RESPONSE_LEN ||
      cursor->header.length < REPLY_HEADER_SIZE) {
    zend_error(E_WARNING, "bad response length: %d\n", cursor->header.length);
    return FAILURE;
  }

  // create buf
  cursor->header.length -= INT_32;
  cursor->buf_start = (char*)emalloc(cursor->header.length);
  // point buf_start at buf's first char
  cursor->buf = cursor->buf_start;

  if (recv(link->socket, cursor->buf, cursor->header.length, FLAGS) == -1) {
    perror("recv");
    return FAILURE;
  }

  memcpy(&cursor->header.request_id, cursor->buf, INT_32);
  cursor->buf += INT_32;
  memcpy(&cursor->header.response_to, cursor->buf, INT_32);
  cursor->buf += INT_32;
  memcpy(&cursor->header.op, cursor->buf, INT_32);
  cursor->buf += INT_32;

  memcpy(&cursor->flag, cursor->buf, INT_32);
  cursor->buf += INT_32;
  memcpy(&cursor->cursor_id, cursor->buf, INT_64);
  cursor->buf += INT_64;
  memcpy(&cursor->start, cursor->buf, INT_32);
  cursor->buf += INT_32;
  memcpy(&cursor->num, cursor->buf, INT_32);
  cursor->buf += INT_32;

  cursor->at = 0;

  return SUCCESS;
}


static int say(mongo_link *link, buffer *buf TSRMLS_DC) {
  if (check_connection(link TSRMLS_CC) == SUCCESS) {
    return send(link->socket, buf->start, buf->pos-buf->start, FLAGS);
  }
  return FAILURE;
}


static int check_connection(mongo_link *link TSRMLS_DC) {
  if (!MonGlo(auto_reconnect) ||
      link->connected || 
      time(0)-link->ts < 2) {
    return SUCCESS;
  }

  link->ts = time(0);

  close(link->socket);
  struct sockaddr_in addr;
  get_sockaddr(&addr, link->host, link->port);
  return link->connected = 
    connect(link->socket, (struct sockaddr*)&addr, sizeof(struct sockaddr));
}

static int mongo_connect(mongo_link *link) {
  int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0) {
    zend_error (E_WARNING, "socket");
    return sock;
  }

  struct sockaddr_in addr;
  get_sockaddr(&addr, link->host, link->port);

  int connected = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  if (connected < 0) {
    zend_error(E_WARNING, "error connecting");
    return sock;
  }

  link->socket = sock;
  link->ts = time(0);
}

static int get_sockaddr(struct sockaddr_in *addr, char *host, int port) {
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  struct hostent *hostinfo = (struct hostent*)gethostbyname(host);
  if (hostinfo == NULL) {
    zend_error (E_WARNING, "unknown host %s", host);
    efree(host);
    return FAILURE;
  }
  addr->sin_addr = *((struct in_addr*)hostinfo->h_addr);
  return SUCCESS;
}


/* {{{ php_mongo_do_connect
 */
static void php_mongo_do_connect(INTERNAL_FUNCTION_PARAMETERS) {
  mongo_link *conn;
  char *server, *uname, *pass, *key;
  zend_bool persistent, paired, lazy;
  int server_len, uname_len, pass_len, key_len;
  zend_rsrc_list_entry *le;
  struct sockaddr_in name;
  
  int argc = ZEND_NUM_ARGS();
  if (argc != 6) {
    zend_error( E_WARNING, "expected 6 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if (zend_parse_parameters(argc TSRMLS_CC, "sssbbb", &server, &server_len, &uname, &uname_len, &pass, &pass_len, &persistent, &paired, &lazy) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected: mongo_pconnect( string, string, string, bool, bool, bool )" );
    RETURN_FALSE;
  }

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

  if (persistent) {
    key_len = spprintf(&key, 0, "%s_%s_%s", server, uname, pass);
    // if a connection is found, return it 
    if (zend_hash_find(&EG(persistent_list), key, key_len+1, (void**)&le) == SUCCESS) {
      conn = (mongo_link*)le->ptr;
      ZEND_REGISTER_RESOURCE(return_value, conn, le_pconnection);
      efree(key);
      return;
    }
    // if lazy and no connection was found, return 
    else if(lazy) {
      efree(key);
      RETURN_NULL();
    }
    efree(key);
  }

  if (server_len == 0) {
    zend_error( E_WARNING, "invalid host" );
    RETURN_FALSE;
  }

  // extract host:port
  char *colon = strchr(server, ':');
  char *host;
  int port;
  if (colon) {
    int host_len = colon-server+1;
    host = (char*)emalloc(host_len);
    memcpy(host, server, host_len-1);
    host[host_len-1] = 0;
    port = atoi(colon+1);
  }
  else {
    host = (char*)emalloc(strlen(server));
    memcpy(host, server, strlen(server));
    port = 27017;
  }

  get_sockaddr(&name, host, port);

  conn = (mongo_link*)emalloc(sizeof(mongo_link));
  // zero pointers so it doesn't segfault on cleanup if connection fails
  conn->username = 0;
  conn->password = 0;
  conn->host = host;
  conn->port = port;

  mongo_connect(conn);

  /*
  if (paired) {
    // we get a string: host1:123,host2:456
    char *comma = strchr(server, ',');
    comma++;
    colon = strchr(server, ':');
    if (colon) {
      int host_len = colon-(comma-server)+1;
      host = (char*)emalloc(host_len);
      memcpy(host, comma-server, host_len-1);
      host[host_len-1] = 0;
      port = atoi(colon+1);
    }
    else {
      host = (char*)emalloc(strlen(server+comma));
      memcpy(host, server, strlen(server+comma));
      port = 27017;
    }

    mongo_link *conn2 = (mongo_link*)emalloc(sizeof(mongo_link));
    mongo_connect(conn2, name);
    efree(host);

    //temp
    efree(conn2);
    }*/
  
  // store a reference in the persistence list
  if (persistent) {
    zend_rsrc_list_entry new_le; 

    // save username and password for reconnection
    if (uname_len > 0 && pass_len > 0) {
      conn->username = (char*)emalloc(uname_len);
      conn->password = (char*)emalloc(pass_len);
      memcpy(conn->username, uname, uname_len);
      memcpy(conn->password, pass, pass_len);
    }

    key_len = spprintf(&key, 0, "%s_%s_%s", server, uname, pass);
    Z_TYPE(new_le) = le_pconnection;
    new_le.ptr = conn;

    if (zend_hash_update(&EG(persistent_list), key, key_len+1, (void*)&new_le, sizeof(zend_rsrc_list_entry), NULL)==FAILURE) { 
      php_connection_dtor(&new_le TSRMLS_CC);
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
