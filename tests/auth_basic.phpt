--TEST--
MongoAuth class - basic authentication functionality
--SKIPIF--
<?php 
include_once "Mongo.php";

$m = new Mongo();
if(!$m->selectCollection("admin", "system.users")->findOne(array("user" => "kristina")))
  die("skip this test if there is no user named \"kristina\"\n");
?>
--FILE--
<?php
include_once "Mongo.php";

$a = new MongoAdmin("kristina", "fred");
echo "$a\n";

/* check it can do everything Mongo can */
$a->selectCollection("phpt", "auth.basic")->insert(array("foo"=>"bar"));
$one = $a->selectCollection("phpt", "auth.basic")->findOne();
echo $one["foo"]."\n";

/* check auth methods */
$a->addUser("fred", "ted");
MongoAuth::getHash("fred", "ted");
$a2 = new MongoAdmin("fred", "ted");
echo "$a\n";

var_dump($a->changePassword("fred", "ted", "foobar"));

$a2 = new MongoAdmin("fred", "ted");
var_dump($a2->loggedIn);

$a2 = new MongoAdmin("fred", "foobar");
var_dump($a2->loggedIn);

$a->deleteUser("fred");
$a2 = new MongoAdmin("fred", "foobar");
var_dump($a2->loggedIn);

?>
--EXPECT--
localhost:27017
bar
localhost:27017
array(1) {
  ["ok"]=>
  float(1)
}
bool(false)
bool(true)
bool(false)
