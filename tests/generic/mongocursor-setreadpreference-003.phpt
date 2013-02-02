--TEST--
MongoCursor::setReadPreference() should allow empty tags parameter for primary mode
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$m = new_mongo_standalone();
$c = $m->phpunit->test->find();
var_dump($c === $c->setReadPreference(Mongo::RP_PRIMARY, array()));
var_dump($c->getReadPreference());
?>
--EXPECT--
bool(true)
array(1) {
  ["type"]=>
  string(7) "primary"
}
