--TEST--
Test for PHP-384: Segfaults with GridFS and long_as_object. (32-bit)
--SKIPIF--
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--INI--
mongo.native_long=1
mongo.long_as_object=0
--FILE--
<?php
require_once dirname(__FILE__) ."/../utils.inc";
$m = mongo();

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
int(4096)
OK
