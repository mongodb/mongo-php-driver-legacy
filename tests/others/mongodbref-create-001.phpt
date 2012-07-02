--TEST--
MongoDBRef::create() without optional database parameter
--SKIPIF--
<?php require __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
var_dump(MongoDBRef::create('foo', 123));
?>
--EXPECT--
array(2) {
  ["$ref"]=>
  string(3) "foo"
  ["$id"]=>
  int(123)
}
