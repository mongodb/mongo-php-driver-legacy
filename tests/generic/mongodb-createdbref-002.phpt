--TEST--
MongoDB::createDBRef() with missing _id field in array/object
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$dsn = MongoShellServer::getStandaloneInfo();
$mongo = new MongoClient($dsn);

var_dump($mongo->database->createDBRef('collection', array()));
var_dump($mongo->database->createDBRef('collection', (object) array()));

?>
--EXPECTF--
NULL
array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(stdClass)#%d (0) {
  }
}

