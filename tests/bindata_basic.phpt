--TEST--
MongoId class - basic id functionality
--FILE--
<?php

include "Mongo.php";

$m=new Mongo();

$bin = new MongoBinData("abcdefg");
var_dump($bin);
$c=$m->selectCollection("phpt", "bindata");
$c->drop();
$c->insert(array("bin"=>$bin));

$obj = $c->findOne();
var_dump($obj["bin"]);

?>
--EXPECT--
object(MongoBinData)#2 (3) {
  ["bin"]=>
  string(7) "abcdefg"
  ["length"]=>
  int(7)
  ["type"]=>
  int(2)
}
object(MongoBinData)#6 (3) {
  ["bin"]=>
  string(7) "abcdefg"
  ["length"]=>
  int(7)
  ["type"]=>
  int(2)
}
