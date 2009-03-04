--TEST--
MongoAuth class - basic authentication functionality
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
Authenticated
bar
