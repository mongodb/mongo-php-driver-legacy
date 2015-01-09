--TEST--
MongoCommandCursor::getReadPreference() inherits value from parent
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php

$mc = new MongoClient(null, array('connect' => false));

// Set before MongoCommandCursor is instantiated
$mc->setReadPreference(MongoClient::RP_PRIMARY_PREFERRED);
$cc = new MongoCommandCursor($mc, 'test.foo', array());
var_dump($cc->getReadPreference());

// Set after MongoCommandCursor is instantiated
$mc->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED);
$cc = new MongoCommandCursor($mc, 'test.foo', array());
$mc->setReadPreference(MongoClient::RP_NEAREST);
var_dump($cc->getReadPreference());

?>
===DONE===
--EXPECT--
array(1) {
  ["type"]=>
  string(16) "primaryPreferred"
}
array(1) {
  ["type"]=>
  string(18) "secondaryPreferred"
}
===DONE===
