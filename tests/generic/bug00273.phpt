--TEST--
Test for PHP-273: MongoCollection::distinct() basic tests
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$db = $m->selectDB(dbname());
$db->dropCollection("addresses");
$c = $db->addresses;

$c->insert(array("stuff" => "bar", "zip-code" => 10010));
$c->insert(array("stuff" => "foo", "zip-code" => 10010));
$c->insert(array("stuff" => "bar", "zip-code" => 99701), array("w" => true));

$retval = $c->distinct("zip-code");
var_dump($retval);

$retval = $c->distinct("zip-code", array("stuff" => "foo"));
var_dump($retval);

$retval = $c->distinct("zip-code", array("stuff" => "bar"));
var_dump($retval);

$c->insert(array("user" => array("points" => 25)));
$c->insert(array("user" => array("points" => 31)));
$c->insert(array("user" => array("points" => 25)), array("w" => true));

$retval = $c->distinct("user.points");
var_dump($retval);
$retval = $c->distinct("user.nonexisting");
var_dump($retval);
?>
--EXPECT--
array(2) {
  [0]=>
  int(10010)
  [1]=>
  int(99701)
}
array(1) {
  [0]=>
  int(10010)
}
array(2) {
  [0]=>
  int(10010)
  [1]=>
  int(99701)
}
array(2) {
  [0]=>
  int(25)
  [1]=>
  int(31)
}
array(0) {
}
