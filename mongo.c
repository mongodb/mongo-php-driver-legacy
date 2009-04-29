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
#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netdb.h>
#endif

#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "mongo.h"
#include "mongo_types.h"
#include "bson.h"

static int get_master(mongo_link* TSRMLS_DC);
static int say(mongo_link*, buffer* TSRMLS_DC);
static int hear(mongo_link*, void*, int TSRMLS_DC);
static int check_connection(mongo_link* TSRMLS_DC);
static int mongo_connect_nonb(int, char*, int);
static int mongo_do_socket_connect(mongo_link*);
static int get_sockaddr(struct sockaddr_in*, char*, int);
static void php_mongo_do_connect(INTERNAL_FUNCTION_PARAMETERS);
static int get_reply(mongo_cursor* TSRMLS_DC);
static void kill_cursor(mongo_cursor* TSRMLS_DC);

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
  mongo_globals->chunk_size = DEFAULT_CHUNK_SIZE;
}
/* }}} */

static void php_connection_dtor( zend_rsrc_list_entry *rsrc TSRMLS_DC ) {
  mongo_link *conn = (mongo_link*)rsrc->ptr;
  if (conn) {
    if (conn->paired) {
      close(conn->server.paired.lsocket);
      close(conn->server.paired.rsocket);

      if (conn->server.paired.left) {
        efree(conn->server.paired.left);
      }
      if (conn->server.paired.right) {
        efree(conn->server.paired.right);
      }
    }
    else {
      // close the connection
      close(conn->server.single.socket);

      // free strings
      if (conn->server.single.host) {
        efree(conn->server.single.host);
      }
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
  // we allocate a cursor even if no results are returned,
  // but the database will throw an assertion if we try to
  // kill a non-existant cursor
  // non-cursors have ids of 0
  if (cursor->cursor_id == 0) {
    return;
  }
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

  say(cursor->link, &buf TSRMLS_CC);
}

static void free_cursor(mongo_cursor *cursor) {
  // free mem
  if (cursor->buf.start) {
    efree(cursor->buf.start);
  }
  if (cursor->ns) {
    efree(cursor->ns);
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

  zend_class_entry regex; 
  INIT_CLASS_ENTRY(regex, "MongoRegex", mongo_regex_functions); 
  mongo_regex_class = zend_register_internal_class(&regex TSRMLS_CC); 

  mongo_init_MongoId(TSRMLS_C);

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

  return SUCCESS;
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
  return_value_ptr = &return_value;
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
  if (!cursor) {
    RETURN_NULL();
  }
  ZEND_REGISTER_RESOURCE(return_value, cursor, le_db_cursor);
}
/* }}} */


/* {{{ mongo_remove
 */
PHP_FUNCTION(mongo_remove) {
  zval *zconn, *zarray;
  char *collection;
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

  mflags |= (justOne == 1);

  serialize_int(&buf, mflags);
  zval_to_bson(&buf, array, NO_PREP TSRMLS_CC);
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
  RETURN_BOOL(response >= SUCCESS);
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

  int count = 0;

  for(zend_hash_internal_pointer_reset_ex(php_array, &pointer); 
      zend_hash_get_current_data_ex(php_array, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(php_array, &pointer)) {

    if(Z_TYPE_PP(data) != IS_ARRAY) {
      efree(buf.start);
      RETURN_FALSE;
    }

    unsigned int start = buf.pos-buf.start;
    zval_to_bson(&buf, Z_ARRVAL_PP(data), NO_PREP TSRMLS_CC);

    serialize_size(buf.start+start, &buf);

    count++;
  }

  // if there are no elements, don't bother saving
  if (count == 0) {
    efree(buf.start);
    RETURN_FALSE;
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
  zval_to_bson(&buf, Z_ARRVAL_P(zquery), NO_PREP TSRMLS_CC);
  zval_to_bson(&buf, Z_ARRVAL_P(zobj), NO_PREP TSRMLS_CC);
  serialize_size(buf.start, &buf);

  RETVAL_BOOL(say(link, &buf TSRMLS_CC)+1);
  efree(buf.start);
}
/* }}} */


/* {{{ mongo_has_next 
 */
PHP_FUNCTION(mongo_has_next) {
  zval *zcursor;
  mongo_cursor *cursor;
  int argc = ZEND_NUM_ARGS();

  if (argc != 1 ) {
    zend_error( E_WARNING, "expected 1 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if( zend_parse_parameters(argc TSRMLS_CC, "r", &zcursor) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected mongo_has_next(cursor)" );
    RETURN_FALSE;
  }

  ZEND_FETCH_RESOURCE(cursor, mongo_cursor*, &zcursor, -1, PHP_DB_CURSOR_RES_NAME, le_db_cursor); 
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

  ZEND_FETCH_RESOURCE(cursor, mongo_cursor*, &zcursor, -1, PHP_DB_CURSOR_RES_NAME, le_db_cursor); 
  zval *z = mongo_do_next(cursor TSRMLS_CC);
  RETURN_ZVAL(z, 0, 1);
}
/* }}} */



/* {{{ mongo_gridfs_init
 */
PHP_FUNCTION(mongo_gridfs_init) {
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
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  if (size >= 0xffffffff) {
    zend_error(E_WARNING, "file %s is too large: %ld bytes\n", filename, size);
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
    chunk_size = size-pos >= MonGlo(chunk_size) ? MonGlo(chunk_size) : size-pos;
    char *buf = (char*)emalloc(chunk_size);
    if (fread(buf, 1, chunk_size, fp) < chunk_size) {
      zend_error(E_WARNING, "Error reading file %s\n", filename);
      RETURN_FALSE;
    }

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
	efree(buf);
  }
  fclose(fp);

  zval *id;
  ALLOC_INIT_ZVAL(id);
  create_id(id, cid TSRMLS_CC);
  add_assoc_zval(zfile, "_id", id);
  add_assoc_stringl(zfile, "filename", filename, filename_len, DUP);
  add_assoc_long(zfile, "length", size);
  add_assoc_long(zfile, "chunkSize", MonGlo(chunk_size));

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
    zval **zdata;
    HashTable *chunk = Z_ARRVAL_P(elem);

    // check if data field exists.  if it doesn't, we've probably
    // got an error message from the db, so return that
    if (zend_hash_find(chunk, "data", 5, (void**)&zdata) == FAILURE) {
      zend_error(E_WARNING, "error reading chunk of file");
      if(zend_hash_exists(chunk, "$err", 5)) {
        free_cursor(c);
        fclose(fp);
        RETURN_ZVAL(elem, 0, 1);
      }
      continue;
    }
    char *data = Z_STRVAL_PP(zdata);
    int len = Z_STRLEN_PP(zdata);

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

  zval_to_bson(&buf, Z_ARRVAL_P(zquery), NO_PREP TSRMLS_CC);
  if (zfields && zend_hash_num_elements(Z_ARRVAL_P(zfields)) > 0) {
    zval_to_bson(&buf, Z_ARRVAL_P(zfields), NO_PREP TSRMLS_CC);
  }

  serialize_size(buf.start, &buf);

  // sends
  int sent = say(link, &buf TSRMLS_CC);
  efree(buf.start);
  if (sent == FAILURE) {
    zend_error(E_WARNING, "couldn't send query\n");
    return 0;
  }

  mongo_cursor *cursor = (mongo_cursor*)emalloc(sizeof(mongo_cursor));
  cursor->buf.start = 0;
  cursor->buf.pos = 0;
  cursor->link = link;
  cursor->ns = estrdup(collection);
  cursor->ns_len = strlen(collection);                        

  get_reply(cursor TSRMLS_CC);

  cursor->limit = limit;
  return cursor;
}


int mongo_do_has_next(mongo_cursor *cursor TSRMLS_DC) {
  if (cursor->num == 0) {
    return 0;
  }
  if (cursor->at < cursor->num) {
    return 1;
  }

  CREATE_BUF(buf, 256);
  CREATE_RESPONSE_HEADER(buf, cursor->ns, cursor->ns_len, cursor->header.request_id, OP_GET_MORE);
  serialize_int(&buf, cursor->limit);
  serialize_long(&buf, cursor->cursor_id);
  serialize_size(buf.start, &buf);
  
  // fails if we're out of elems
  if(say(cursor->link, &buf TSRMLS_CC) == FAILURE) {
    efree(buf.start);
    return 0;
  }
  
  efree(buf.start);

  // if we have cursor->at == cursor->num && recv fails,
  // we're probably just out of results
  return (get_reply(cursor TSRMLS_CC) == SUCCESS);
}


zval* mongo_do_next(mongo_cursor *cursor TSRMLS_DC) {
  if (cursor->at >= cursor->num) {
    // check for more results
    if (!mongo_do_has_next(cursor TSRMLS_CC)) {
      // we're out of results
      return 0;
    }
    // we got more results
  }

  if (cursor->at < cursor->num) {
    zval *elem;
    ALLOC_INIT_ZVAL(elem);
    array_init(elem);
    cursor->buf.pos = bson_to_zval(cursor->buf.pos, elem TSRMLS_CC);

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
  // serialize
  if (zval_to_bson(&buf, obj, PREP TSRMLS_CC) == 0) {
    // return if there were 0 elements
    return FAILURE;
  }

  serialize_size(buf.start, &buf);

  // sends
  int response = say(link, &buf TSRMLS_CC);
  efree(buf.start);
  return response;
}

static int get_master(mongo_link *link TSRMLS_DC) {
  if (!link->paired) {
    return link->server.single.socket;
  }

  if (link->server.paired.lsocket == link->master &&
      link->server.paired.lconnected) {
    return link->server.paired.lsocket;
  }
  else if (link->server.paired.rsocket == link->master &&
           link->server.paired.rconnected) {
    return link->server.paired.rsocket;
  }

  // redetermine master
  zval *is_master, *response;
  ALLOC_INIT_ZVAL(is_master);
  array_init(is_master);
  add_assoc_long(is_master, "ismaster", 1);

  mongo_link temp;
  temp.paired = 0;
  temp.server.single.socket = link->server.paired.lsocket;

  // check the left
  mongo_cursor *c = mongo_do_query(&temp, "admin.$cmd", 0, -1, is_master, 0 TSRMLS_CC);

  if ((response = mongo_do_next(c TSRMLS_CC)) != 0) {
    zval **ans;
    if (zend_hash_find(Z_ARRVAL_P(response), "ismaster", 9, (void**)&ans) == SUCCESS && 
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&is_master);
      return link->master = link->server.paired.lsocket;
    }
  }

  // check the right
  temp.server.single.socket = link->server.paired.rsocket;

  c = mongo_do_query(&temp, "admin.$cmd", 0, -1, is_master, 0 TSRMLS_CC);

  if ((response = mongo_do_next(c TSRMLS_CC)) != 0) {
    zval **ans;
    if (zend_hash_find(Z_ARRVAL_P(response), "ismaster", 9, (void**)&ans) == SUCCESS && 
        Z_LVAL_PP(ans) == 1) {
      zval_ptr_dtor(&is_master);
      return link->master = link->server.paired.rsocket;
    }
  }

  // give up
  zval_ptr_dtor(&is_master);
  return FAILURE;
}


static int get_reply(mongo_cursor *cursor TSRMLS_DC) {
  if(cursor->buf.start) {
    efree(cursor->buf.start);
    cursor->buf.start = 0;
  }
  
  // if this fails, we might be disconnected... but we're probably
  // just out of results
  if (hear(cursor->link, &cursor->header.length, INT_32 TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  // make sure we're not getting crazy data
  if (cursor->header.length > MAX_RESPONSE_LEN ||
      cursor->header.length < REPLY_HEADER_SIZE) {
    zend_error(E_WARNING, "bad response length: %d, did the db assert?\n", cursor->header.length);
    return FAILURE;
  }

  // create buf
  cursor->header.length -= INT_32;
  // point buf.start at buf's first char
  cursor->buf.start = (unsigned char*)emalloc(cursor->header.length);
  cursor->buf.pos = cursor->buf.start;
  cursor->buf.end = cursor->buf.start + cursor->header.length;

  if (hear(cursor->link, cursor->buf.pos, cursor->header.length TSRMLS_CC) == FAILURE) {
    zend_error(E_WARNING, "error getting response buf: %s\n", strerror(errno));
    return FAILURE;
  }

  memcpy(&cursor->header.request_id, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  memcpy(&cursor->header.response_to, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  memcpy(&cursor->header.op, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;

  memcpy(&cursor->flag, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  if(cursor->flag == CURSOR_NOT_FOUND) {
    cursor->num = 0;
    return FAILURE;
  }

  memcpy(&cursor->cursor_id, cursor->buf.pos, INT_64);
  cursor->buf.pos += INT_64;
  memcpy(&cursor->start, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;
  memcpy(&cursor->num, cursor->buf.pos, INT_32);
  cursor->buf.pos += INT_32;

  cursor->at = 0;

  return cursor->num == 0 ? FAILURE : SUCCESS;
}


static int say(mongo_link *link, buffer *buf TSRMLS_DC) {
  if (check_connection(link TSRMLS_CC) == SUCCESS) {

#ifdef WIN32
    int sent = send(get_master(link TSRMLS_CC), (const char*)buf->start, buf->pos-buf->start, FLAGS);
#else
	int sent = send(get_master(link TSRMLS_CC), buf->start, buf->pos-buf->start, FLAGS);
#endif

    if (sent == FAILURE) {
      link->server.single.connected = 0;
      sent = check_connection(link TSRMLS_CC);
    }
    return sent;
  }
  return FAILURE;
}

static int hear(mongo_link *link, void *dest, int len TSRMLS_DC) {
  int tries = 3, num = 1;
  while (check_connection(link TSRMLS_CC) == FAILURE &&
         tries > 0) {
#ifndef WIN32
    sleep(2);
#endif
    zend_error(E_WARNING, "no connection, trying to reconnect %d more times...\n", tries--);
  }
  // this can return FAILED if there is just no more data from db
  int r = 0;
  while(r < len && num > 0) {

#ifdef WIN32
    num = recv(get_master(link TSRMLS_CC), (char*)dest, len, FLAGS);
#else
    num = recv(get_master(link TSRMLS_CC), dest, len, FLAGS);
#endif

	dest = (char*)dest + num;
    r += num;
  }
  return r;
}

static int check_connection(mongo_link *link TSRMLS_DC) {
  if (!MonGlo(auto_reconnect) ||
      (!link->paired && link->server.single.connected) || 
      (link->paired && 
       (link->server.paired.lconnected || 
        link->server.paired.rconnected)) ||
      time(0)-link->ts < 2) {
    return SUCCESS;
  }

  link->ts = time(0);

  if (link->paired) {
    close(link->server.paired.lsocket);
    close(link->server.paired.rsocket);
  }
  else {
    close(link->server.single.socket);
  } 

  return mongo_do_socket_connect(link);
}

static int mongo_connect_nonb(int sock, char *host, int port) {
  struct sockaddr_in addr, addr2;
  fd_set rset, wset, eset;
  struct timeval tval;
#ifdef WIN32
  const char yes = 1;
#else
  int yes = 1;
#endif

  int connected = FAILURE;

  // timeout
  tval.tv_sec = 20;
  tval.tv_usec = 0;

  // get addresses
  get_sockaddr(&addr, host, port);

  // create sockets
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == FAILURE) {
    zend_error (E_WARNING, "couldn't create socket");
    return FAILURE;
  }

  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, INT_32);
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, INT_32);
#ifdef WIN32
  ioctlsocket(sock, FIONBIO, (u_long*)&yes);
#else
  fcntl(sock, F_SETFL, FLAGS|O_NONBLOCK);
#endif

  FD_ZERO(&rset);
  FD_SET(sock, &rset);  
  FD_ZERO(&wset);
  FD_SET(sock, &wset);  

  // connect
  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef WIN32
    errno = WSAGetLastError();
    if (errno != WSAEINPROGRESS) {
#else
    if (errno != EINPROGRESS) {
#endif
      close(sock);
      zend_error(E_WARNING, "%s:%d: %s", host, port, strerror(errno));
      return FAILURE;
    }

    if (select(sock+1, &rset, &wset, NULL, &tval) == 0) {
      close(sock);
      return FAILURE;
    }

    int size = sizeof(addr2);
    connected = getpeername(sock, (struct sockaddr*)&addr, &size);
    if (connected == FAILURE) {
      close(sock);
      return FAILURE;
    }
  }

  // reset flags
#ifdef WIN32
  u_long no = 0;
  ioctlsocket(sock, FIONBIO, &no);
#else
  fcntl(sock, F_SETFL, FLAGS);
#endif

  return sock;
}

static int mongo_do_socket_connect(mongo_link *link) {
  if (link->paired) {

    if ((link->server.paired.lsocket =
         mongo_connect_nonb(link->server.paired.lsocket,
                            link->server.paired.left,
                            link->server.paired.lport)) == FAILURE) {

      link->server.paired.lconnected = 0;
    }
    else {
      link->server.paired.lconnected = 1;
    }
    if ((link->server.paired.rsocket =
         mongo_connect_nonb(link->server.paired.lsocket,
                            link->server.paired.left,
                            link->server.paired.lport)) == FAILURE) {
      link->server.paired.rconnected = 0;
    }
    else {
      link->server.paired.rconnected = 1;
    }

    if (!link->server.paired.lconnected &&
        !link->server.paired.rconnected) {
      return FAILURE;
    }
  }
  else {
    if ((link->server.single.socket = 
         mongo_connect_nonb(link->server.single.socket,
                            link->server.single.host,
                            link->server.single.port)) == FAILURE) {
      link->server.single.connected = 0;
      return FAILURE;
    }
    link->server.single.connected = 1;
  }

  // set initial connection time
  link->ts = time(0);

  return SUCCESS;
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
  
  int argc = ZEND_NUM_ARGS();
  if (argc != 6) {
    zend_error( E_WARNING, "expected 6 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }
  else if (zend_parse_parameters(argc TSRMLS_CC, "sssbbb", &server, &server_len, &uname, &uname_len, &pass, &pass_len, &persistent, &paired, &lazy) == FAILURE) {
    zend_error( E_WARNING, "incorrect parameter types, expected: mongo_connect( string, string, string, bool, bool, bool )" );
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


  conn = (mongo_link*)emalloc(sizeof(mongo_link));

  // zero pointers so it doesn't segfault on cleanup if 
  // connection fails
  conn->username = 0;
  conn->password = 0;


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
    host = estrdup(server);
    port = 27017;
  }

  if (paired) {
    conn->paired = 1;

    conn->server.paired.left = host;
    conn->server.paired.lport = port;


    // we get a string: host1:123,host2:456
    char *comma = strchr(server, ',');
    comma++;
    colon = strchr(comma, ':');

    // check that length is sane
    if (colon && 
        colon - comma > 0 &&
        colon - comma < 256) {
      int host_len = colon-comma + 1;
      host = (char*)emalloc(host_len);
      memcpy(host, comma, host_len-1);
      host[host_len-1] = 0;
      port = atoi(colon + 1);
    }
    else {
      host = (char*)emalloc(strlen(comma)+1);
      memcpy(host, comma, strlen(comma));
      host[strlen(comma)] = 0;
      port = 27017;
    }

    conn->server.paired.right = host;
    conn->server.paired.rport = port;
  }
  else {
    conn->paired = 0;

    conn->server.single.host = host;
    conn->server.single.port = port;
  }

  // actually connect
  if (mongo_do_socket_connect(conn) == FAILURE) {
    RETURN_FALSE;
  }

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
