--TEST--
Test for PHP-429: MongoDB::selectCollection() causes Segmentation fault. (2)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php require_once "tests/utils/server.inc";

$mongo = new_mongo_standalone();

$db = $mongo->selectDB(dbname());

class foo {
    function __toString() {
        return "foo";
    }
}
$f = new foo;
$m = new MongoCollection($db, $f);
var_dump($m);

$m2 = new MongoCollection($db, $m);
var_dump($m2);

$col1 = $db->selectCollection($f);
var_dump($col1);

?>
===DONE===
<?php exit(0);?>
--EXPECTF--
object(MongoCollection)#4 (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(MongoCollection)#5 (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(MongoCollection)#6 (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
===DONE===

