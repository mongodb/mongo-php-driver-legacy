--TEST--
Test for PHP-xxx: Description
--SKIPIF--
<?php $needs = "2.0.0"; $needsOp = "gt"; ?>
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$s = new MongoShellServer;
$cfg = $s->getReplicaSetConfig();
$opts = array('replicaSet' => $cfg['rsname']);
$mc = new MongoClient($cfg["dsn"], $opts);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

echo "My test here\n";
$s->killMaster();

?>
===DONE===
<?php exit(0); ?>
--CLEAN--
<?php require_once "tests/utils/fix-master.inc"; ?>
--EXPECTF--
My test here
===DONE===

