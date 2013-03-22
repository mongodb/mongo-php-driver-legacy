--TEST--
MongoDB::getReadPreference() inherits value from parent
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

// Set before MongoDB is instantiated
$m = new_mongo_standalone(null, true, true, array('readPreference' => MongoClient::RP_SECONDARY_PREFERRED));
$m->setReadPreference(Mongo::RP_PRIMARY_PREFERRED);
$db = $m->phpunit;
var_dump($db->getReadPreference());

// Set after MongoDB is instantiated
$m = new_mongo_standalone(null, true, true, array('readPreference' => MongoClient::RP_SECONDARY_PREFERRED));
$db = $m->phpunit;
$m->setReadPreference(Mongo::RP_SECONDARY);
var_dump($db->getReadPreference());
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
