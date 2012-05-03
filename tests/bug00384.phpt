--TEST--
Test for PHP-384: Segfaults with GridFS and long_as_object.
--INI--
mongo.long_as_object=0
--FILE--
<?php
$m = new Mongo();
$m->phpunit->dropCollection( 'fs.files' );
$m->phpunit->dropCollection( 'fs.chunks' );

$g = $m->phpunit->getGridFS();
$id = $g->storeBytes( str_repeat("\0", 4096 ) );


ini_set( 'mongo.long_as_object', 0 );
$cursor = $g->get( $id );
$a = $cursor->getBytes();
var_dump( $cursor->getSize() );

ini_set( 'mongo.long_as_object', 1 );
$cursor = $g->get( $id );
$a = $cursor->getBytes();
var_dump( $cursor->getSize() );

echo 'OK'. PHP_EOL;
?>
--EXPECTF--
int(4096)
object(MongoInt64)#12 (1) {
  ["value"]=>
  string(4) "4096"
}
OK
