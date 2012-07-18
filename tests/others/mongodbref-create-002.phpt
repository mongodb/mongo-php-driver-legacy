--TEST--
MongoDBRef::create() with optional database parameter
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
var_dump(MongoDBRef::create('foo', 123, 'bar'));
?>
--EXPECT--
array(3) {
  ["$ref"]=>
  string(3) "foo"
  ["$id"]=>
  int(123)
  ["$db"]=>
  string(3) "bar"
}
