--TEST--
Test for PHP-xxx: Description
--SKIPIF--
<?php $needs = "2.0.0"; $needsOp = "gt"; ?>
<?php require_once "tests/utils/bridge.inc" ?>
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getBridgeInfo();
$mc = new MongoClient($host);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

echo "My test here\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
My test here
===DONE===

