//mongo_id.cpp

#include <php.h>
#include <mongo/client/dbclient.h>
#include <string.h>

#include "mongo.h"

extern zend_class_entry *mongo_id_class;

PHP_FUNCTION( mongo_id___construct ) {
  mongo::OID *oid = new mongo::OID();
  oid->init();
  std::string str = oid->str();
  char *c = (char*)str.c_str();
  add_property_stringl( getThis(), "id", c, strlen( c ), 1 );
}

PHP_FUNCTION( mongo_id___toString ) {
  zval *zid = zend_read_property( mongo_id_class, getThis(), "id", 2, 0 TSRMLS_CC );
  char *id = Z_STRVAL_P( zid );
  RETURN_STRING( id, 1 );
}

zval* oid_to_mongo_id( mongo::OID oid ) {
  TSRMLS_FETCH();
  zval *zoid;
  
  std::string str = oid.str();
  char *c = (char*)str.c_str();
  
  MAKE_STD_ZVAL(zoid);
  object_init_ex(zoid, mongo_id_class);
  add_property_stringl( zoid, "id", c, strlen( c ), 1 );
  return zoid;
}
