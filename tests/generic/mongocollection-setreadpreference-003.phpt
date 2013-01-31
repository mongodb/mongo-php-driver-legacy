--TEST--
MongoCollection::setReadPreference() should allow empty tags parameter for primary mode
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$m = new_mongo_standalone();
$c = $m->phpunit->test;
var_dump($c->setReadPreference(Mongo::RP_PRIMARY, array()));
var_dump($c->getReadPreference());
?>
--EXPECT--
bool(true)
array(1) {
  ["type"]=>
  string(7) "primary"
}
