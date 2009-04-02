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
  ["length"]=>
  int(7)
  ["bin"]=>
  string(7) "abcdefg"
  ["type"]=>
  int(2)
}
object(MongoBinData)#7 (3) {
  ["length"]=>
  int(7)
  ["bin"]=>
  string(7) "abcdefg"
  ["type"]=>
  int(2)
}
