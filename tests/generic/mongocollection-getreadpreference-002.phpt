--TEST--
MongoCollection::getReadPreference() inherits value from parent
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

// Set before MongoCollection is instantiated
$m = new_mongo(null, true, true, array('readPreference' => MongoClient::RP_SECONDARY_PREFERRED));
$db = $m->phpunit;
$db->setReadPreference(Mongo::RP_PRIMARY_PREFERRED);
$c = $db->test;
var_dump($c->getReadPreference());

// Set after MongoCollection is instantiated
$m = new_mongo(null, true, true, array('readPreference' => MongoClient::RP_SECONDARY_PREFERRED));
$db = $m->phpunit;
$c = $db->test;
$db->setReadPreference(Mongo::RP_SECONDARY);
var_dump($c->getReadPreference());
?>
--EXPECT--
array(1) {
  ["type"]=>
  string(16) "primaryPreferred"
}
array(1) {
  ["type"]=>
  string(18) "secondaryPreferred"
}
