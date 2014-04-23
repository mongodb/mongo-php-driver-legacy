--TEST--
MongoInt64 value property read-only
--SKIPIF--
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
$int = new MongoInt64("85876234523452345");
var_dump($int);
$int->value = 3.1415;
var_dump($int);
?>
--EXPECTF--
object(MongoInt64)#%d (1) {
  ["value"]=>
  string(17) "85876234523452345"
}

Deprecated: main(): The 'value' property is read-only in %smongoint64.php on line %d
object(MongoInt64)#%d (1) {
  ["value"]=>
  string(17) "85876234523452345"
}
