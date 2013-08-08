--TEST--
MongoDBRef::create()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$dsn = MongoShellServer::getStandaloneInfo();
$mongo = new MongoClient($dsn);
$id = new MongoId('5202e48be84df152458b4567');

var_dump(MongoDBRef::create('collection', $id));
var_dump(MongoDBRef::create('collection', array('_id' => $id)));

var_dump(MongoDBRef::create('collection', $id, 'database'));
var_dump(MongoDBRef::create('collection', array('_id' => $id), 'database'));

?>
--EXPECTF--
array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5202e48be84df152458b4567"
  }
}
array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5202e48be84df152458b4567"
  }
}
array(3) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5202e48be84df152458b4567"
  }
  ["$db"]=>
  string(8) "database"
}
array(3) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5202e48be84df152458b4567"
  }
  ["$db"]=>
  string(8) "database"
}
