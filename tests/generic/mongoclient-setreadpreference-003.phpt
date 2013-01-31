--TEST--
MongoClient::setReadPreference() should allow empty tags parameter for primary mode
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$m = new_mongo_standalone();
var_dump($m->setReadPreference(Mongo::RP_PRIMARY, array()));
var_dump($m->getReadPreference());
?>
--EXPECT--
bool(true)
array(1) {
  ["type"]=>
  string(7) "primary"
}
