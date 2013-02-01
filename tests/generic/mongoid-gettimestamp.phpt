--TEST--
MongoID: Test getting the getTimestamp.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
// Connect
$m = mongo_standalone();
// Select the DB
$db = $m->phpunit;
// Select a collection 
$collection = $db->test;

$collection->drop();

// Add a record 
$obj = array("title" => "test1");

$time = time();
$collection->insert($obj);

$cursor = $collection->find();

$a = $cursor->getNext();

var_dump($time === $a['_id']->getTimestamp());
?>
--EXPECTF--
bool(true)
