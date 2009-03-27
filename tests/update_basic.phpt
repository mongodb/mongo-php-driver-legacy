--TEST--
MongoCollection::update - basic update functionality
--FILE--
<?php

include "Mongo.php";

$m=new Mongo();
$c=$m->selectCollection("phpt", "update");
$c->drop();

$old = array("foo"=>"bar", "x"=>"y");
$new = array("foo"=>"baz");

$c->update(array("foo"=>"bar"), $old, true);
var_dump($c->findOne());

$c->update($old, $new);
var_dump($c->findOne());
?>
--EXPECTF--
array(3) {
  ["_id"]=>
  object(MongoId)#5 (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["foo"]=>
  string(3) "bar"
  ["x"]=>
  string(1) "y"
}
array(2) {
  ["_id"]=>
  object(MongoId)#4 (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["foo"]=>
  string(3) "baz"
}
