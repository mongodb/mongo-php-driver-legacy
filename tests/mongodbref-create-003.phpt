--TEST--
MongoDBRef::create() casts reference and database parameters to strings
--FILE--
<?php
var_dump(MongoDBRef::create(123, 456, 789));
?>
--EXPECT--
array(3) {
  ["$ref"]=>
  string(3) "123"
  ["$id"]=>
  int(456)
  ["$db"]=>
  string(3) "789"
}
