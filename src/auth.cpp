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
#include <mongo/client/dbclient.h>

#include "mongo.h"
#include "auth.h"

extern int le_connection;

/* {{{ proto resource mongo_auth_connect(resource connection, string username, string password)
   Create and store a persistent authenticated connection.
 */
PHP_FUNCTION(mongo_auth_connect) {
  zval *zconn;
  char *uname, *pass, *key;
  int uname_len, pass_len, key_len;
  list_entry le;
  mongo::DBClientConnection *c;

  int argc = ZEND_NUM_ARGS();
  if (argc != 3 ||
      zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zconn, &uname, &uname_len, &pass, &pass_len) == FAILURE) {
    zend_error( E_WARNING, "parameter parse failure, expected: mongo_auth_connect( resource, string, string )" );
    RETURN_FALSE;
  }

  // check if a connection already exists
  /*  if(c = get_auth_conn(uname, pass)) { 
    ZEND_REGISTER_RESOURCE(return_value, c, le_connection_p);
    return;
    }*/

  // create a new persistent connection
  c = (mongo::DBClientConnection*)zend_fetch_resource(&zconn TSRMLS_CC, -1, PHP_CONNECTION_RES_NAME, NULL, 1, le_connection);
  if (!c) {
    zend_error( E_WARNING, "no db connection" );
    RETURN_FALSE;
  }

  mongo::DBClientConnection *c2 = new mongo::DBClientConnection(c);
  // store a reference in the persistence list
  key_len = spprintf(&key, 0, "conn_%s_%s", uname, pass);
  le.ptr = c2;
  //  le.type = le_connection_p;
  zend_hash_add(&EG(persistent_list), key, key_len + 1, &le, sizeof(list_entry), NULL);
  efree(key);

  // remove the transitory connection
  zend_list_delete(Z_LVAL_P(zconn));
  // save the persistent one
  //  ZEND_REGISTER_RESOURCE(return_value, c2, le_connection_p);
}
/* }}} */


/* {{{ proto resource mongo_auth_get(string username, string password)
   Gets a persistent authenticated connection.
 */
PHP_FUNCTION(mongo_auth_get) {
  mongo::DBClientConnection *conn;
  char *uname, *pass;
  int uname_len, pass_len;

  int argc = ZEND_NUM_ARGS();
  if (argc != 2 ||
      zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &uname, &uname_len, &pass, &pass_len) == FAILURE) {
    zend_error( E_WARNING, "parameter parse failure, expected: mongo_auth_get( string, string )" );
    RETURN_FALSE;
  }

  conn = get_auth_conn(uname, pass);
  if (!conn) {
    RETURN_NULL();
  }
  //  ZEND_REGISTER_RESOURCE(return_value, conn, le_connection_p);
}
/* }}} */


/* {{{ proto resource mongo_auth_close()
   Closes a persistent authenticated connection.
 */
/*PHP_FUNCTION(mongo_auth_close) {
  
  }*/

mongo::DBClientConnection* get_auth_conn(char *username, char *password) {
  TSRMLS_FETCH();
  char *key;
  int key_len;
  list_entry *le;
  void *foo;

  key_len = spprintf(&key, 0, "conn_%s_%s", username, password);
  if (zend_hash_find(&EG(persistent_list), key, key_len + 1, &foo) == SUCCESS) {
    le = (list_entry*)foo;
    efree(key);
    return (mongo::DBClientConnection*)le->ptr;
  }
  efree(key);
  return NULL;
}
