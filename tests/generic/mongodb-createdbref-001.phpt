--TEST--
MongoDB::createDBRef()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$dsn = MongoShellServer::getStandaloneInfo();
$mongo = new MongoClient($dsn);
$id = new MongoId('5202e48be84df152458b4567');

var_dump($mongo->database->createDBRef('collection', $id));
var_dump($mongo->database->createDBRef('collection', array('_id' => $id)));

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
