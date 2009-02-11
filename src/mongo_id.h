
#include <php.h>

PHP_FUNCTION( mongo_id___construct );
PHP_FUNCTION( mongo_id___toString );
zval* oid_to_mongo_id( mongo::OID oid );
