--TEST--
Test for PHP-429: MongoDB::selectCollection() causes Segmentation fault.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php require_once "tests/utils/server.inc";

$mongo = new_mongo_standalone();
$database = $mongo->selectDB(dbname());

$collection = $database->selectCollection("test");
var_dump($collection);
$second_collection = $database->selectCollection($collection);
var_dump($second_collection);
var_dump($collection);

$collection->find();
?>
===DONE===
<?php exit(0);?>
--EXPECTF--
object(MongoCollection)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(MongoCollection)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(MongoCollection)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
===DONE===

