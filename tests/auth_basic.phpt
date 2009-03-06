--TEST--
MongoAuth class - basic authentication functionality
--SKIPIF--
<?php 
include "mongo.php";

$m = new Mongo();
if(!$m->selectCollection("admin", "system.users")->findOne(array("user" => "kristina")))
  die("skip this test if there is no user named \"kristina\"\n");
?>
--FILE--
<?php
include "mongo.php";

$a = new MongoAdmin("localhost", 27017, "kristina", "fred");
echo "$a\n";

$a->selectCollection("phpt", "auth.basic")->insert(array("foo"=>"bar"));
$one = $a->selectCollection("phpt", "auth.basic")->findOne();
echo $one["foo"]."\n";

?>
--EXPECT--
localhost:27017
bar
