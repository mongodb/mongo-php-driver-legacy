// bson.c

// note: key size is limited to 256 characters

#include <php.h>
#include <mongo/client/dbclient.h>

extern int le_db_oid;

void php_array_to_bson( mongo::BSONObjBuilder *obj_builder, HashTable *arr_hash ) {
  zval **data;
  char *key;
  uint key_len;
  ulong index;
  zend_bool duplicate = 0;
  HashPosition pointer;

  //  mongo::BSONObjBuilder obj_builder;

  for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer); 
      zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS; 
      zend_hash_move_forward_ex(arr_hash, &pointer)) {

    int key_type = zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, duplicate, &pointer);
    char field_name[256];

    // make \0 safe
    if( key_type == HASH_KEY_IS_STRING ) {
      strcpy( field_name, key );
    }
    else if( key_type == HASH_KEY_IS_LONG ) {
      sprintf( field_name, "%ld", index );
    }
    else {
      zend_error( E_ERROR, "key fail" );
      break;
    }

    switch (Z_TYPE_PP(data)) {
    case IS_NULL:
      obj_builder->appendNull( field_name );
      break;
    case IS_LONG:
      obj_builder->append( field_name, Z_LVAL_PP(data) );
      break;
    case IS_DOUBLE:
      obj_builder->append( field_name, Z_DVAL_PP(data) );
      break;
    case IS_BOOL:
      obj_builder->append( field_name, Z_BVAL_PP(data) );
      break;
    case IS_ARRAY: {
      mongo::BSONObjBuilder *subobj = new mongo::BSONObjBuilder();
      php_array_to_bson( subobj, Z_ARRVAL_PP( data ) );
      obj_builder->append( field_name, subobj->doneAndDecouple() );
      break;
    }
    // this should probably be done as bin data, to guard against \0
    case IS_STRING:
      obj_builder->append( field_name, Z_STRVAL_PP(data) );
      break;
    case IS_RESOURCE:
    case IS_CONSTANT:
    case IS_CONSTANT_ARRAY:
    case IS_OBJECT:
    default:
      php_printf( "php=>bson: type %i not supported", Z_TYPE_PP(data) );
    }
    php_printf("\n");
  }
}

zval *bson_to_php_array( mongo::BSONObj obj ) {
  zval *array;
  ALLOC_INIT_ZVAL( array );
  array_init(array);

  mongo::BSONObjIterator it = mongo::BSONObjIterator( obj );
  while( it.more() ) {
    mongo::BSONElement elem = it.next();

    char *key = (char*)elem.fieldName();
    int index = atoi( key );
    // check if 0 index is valid, or just a failed 
    // string conversion
    if( index == 0 && strcmp( "0", key ) != 0 ) {
      index = -1;
    }
    int assoc = index == -1;

    switch( elem.type() ) {
    case mongo::jstNULL: {
      if( assoc )
        add_assoc_null( array, key );
      else 
        add_index_null( array, index );
      break;
    }
    case mongo::NumberInt: {
      long num = (long)elem.number();
      if( assoc )
        add_assoc_long( array, key, num );
      else 
        add_index_long( array, index, num );
      break;
    }
    case mongo::NumberDouble: {
      double num = elem.number();
      if( assoc )
        add_assoc_double( array, key, num );
      else 
        add_index_double( array, index, num );
      break;
    }
    case mongo::Bool: {
      int b = elem.boolean();
      if( assoc )
        add_assoc_bool( array, key, b );
      else 
        add_index_bool( array, index, b );
      break;
    }
    case mongo::String: {
      char *value = (char*)elem.valuestr();
      if( assoc ) 
        add_assoc_string( array, key, value, 1 );
      else 
        add_index_string( array, index, value, 1 );
      break;
    }
    case mongo::Array:
    case mongo::Object: {
      zval *subarray = bson_to_php_array( elem.embeddedObject() );
      if( assoc ) 
        add_assoc_zval( array, key, subarray );
      else 
        add_index_zval( array, index, subarray );
      break;
    }
    case mongo::jstOID: {
      mongo::OID oid = elem.__oid();
      ZEND_REGISTER_RESOURCE( NULL, &oid, le_db_oid );
      if( assoc ) 
        add_assoc_resource( array, key, le_db_oid );
      else 
        add_index_resource( array, index, le_db_oid );
      break;
    }
    case mongo::EOO: {
      break;
    }
    default:
      php_printf( "bson=>php: type %i not supported\n", elem.type() );
    }
  }
  return array;
}

void prep_obj_for_db( mongo::BSONObjBuilder *array ) {
  mongo::OID *oid = new mongo::OID();
  oid->init();
  array->appendOID( "_id", oid);
}
