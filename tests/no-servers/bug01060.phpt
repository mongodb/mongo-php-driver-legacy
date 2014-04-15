--TEST--
Test for PHP-1060: The '$id' property is read-only
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
class MyId extends MongoId {
}

$a = new MyId;
var_dump( $a );
?>
--EXPECTF--
object(MyId)#%d (%d) {
  ["$id"]=>
  string(24) "%s"
}
