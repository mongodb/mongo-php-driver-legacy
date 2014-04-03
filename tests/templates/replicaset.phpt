--TEST--
Test for PHP-xxx: Description
--SKIPIF--
<?php $needs = "2.0.0"; $needsOp = "gt"; ?>
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cfg = MongoShellServer::getReplicaSetInfo();
$opts = array('replicaSet' => $cfg['rsname']);
$mc = new MongoClient($cfg["dsn"], $opts);
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

