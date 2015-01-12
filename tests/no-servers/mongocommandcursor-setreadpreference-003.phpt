--TEST--
MongoCommandCursor::setReadPreference() should allow empty tags parameter for primary mode
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php

$mc = new MongoClient(null, array('connect' => false));
$cc = new MongoCommandCursor($mc, 'test.foo', array());
var_dump($cc === $cc->setReadPreference(MongoClient::RP_PRIMARY, array()));
var_dump($cc->getReadPreference());

?>
===DONE===
--EXPECT--
bool(true)
array(1) {
  ["type"]=>
  string(7) "primary"
}
===DONE===
