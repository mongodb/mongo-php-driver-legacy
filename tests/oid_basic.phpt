--TEST--
MongoId class - basic id functionality
--FILE--
<?php

include "Mongo.php";

$id = new MongoId("49b16fab04bc94cc343c5482");
var_dump($id);

echo "$id\n";

?>
--EXPECTF--
object(MongoId)#1 (1) {
  ["id"]=>
  string(12) "%s"
}
49b16fab04bc94cc343c5482
