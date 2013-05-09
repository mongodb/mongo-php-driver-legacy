--TEST--
Test for PHP-635: The second argument to MongoCursor::__construct() always gets converted to a string.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$collection = $m->selectDb(dbname())->bug635;
var_dump($collection);
 
$cursor = new MongoCursor($m, $collection);
var_dump($collection);
var_dump(is_object($collection));
?>
--EXPECTF--
object(MongoCollection)#%d (%d) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(MongoCollection)#%d (%d) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
bool(true)
