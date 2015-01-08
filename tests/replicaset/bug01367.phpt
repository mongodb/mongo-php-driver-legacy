--TEST--
Test for PHP-1367: aggregate() with var args does not restore read pref after forcing primary for $out
--INI--
error_reporting=-1
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::RS, MongoLog::WARNING, '/Forcing aggregate/');

$rs = MongoShellServer::getReplicasetInfo();

$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$coll = $mc->selectCollection(dbname(), collname(__FILE__));
$coll->drop();

$collOut = $mc->selectCollection(dbname(), collname(__FILE__) . '-out');
$collOut->drop();

$coll->insert(array('_id' => 1));
$coll->insert(array('_id' => 2));

$coll->setReadPreference(MongoClient::RP_SECONDARY);

$rp = $coll->getReadPreference();
printf("Collection read preference: %s\n", $rp['type']);

$coll->aggregate(
    array('$match' => array('_id' => 2)),
    array('$out' => $collOut->getName())
);

printf("Output collection count: %d\n", $collOut->count());

$rp = $coll->getReadPreference();
printf("Collection read preference: %s\n", $rp['type']);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Collection read preference: secondary
Forcing aggregate with $out to run on primary

Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
Output collection count: 1
Collection read preference: secondary
===DONE===
