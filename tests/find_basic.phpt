--TEST--
MongoCollection::find - basic find functionality
--FILE--
<?php

include "Mongo.php";

$m=new Mongo();
$c=$m->selectCollection("phpt", "find");
$c->drop();

$c->insert(array("foo" => "bar",
                 "a" => "b",
                 "b" => "c"));

$cursor = $c->find(array("foo"=>"bar"), array("a"=>1,"b"=>1));

while ($cursor->hasNext()) {
  var_dump($cursor->getNext());
}

?>
--EXPECTF--
array(3) {
  ["_id"]=>
  object(MongoId)#5 (1) {
    ["id"]=>
    string(24) "%x"
  }
  ["a"]=>
  string(1) "b"
  ["b"]=>
  string(1) "c"
}
