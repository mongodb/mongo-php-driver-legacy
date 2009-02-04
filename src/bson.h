
#include <mongo/client/dbclient.h>

void php_array_to_bson( mongo::BSONObjBuilder *obj, HashTable *arr_hash );
zval *bson_to_php_array( mongo::BSONObj obj );
void prep_obj_for_db( mongo::BSONObjBuilder *obj );
