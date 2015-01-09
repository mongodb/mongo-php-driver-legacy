--TEST--
Test for PHP-1369: Clear tag sets when forcing primary read pref for aggregate
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

$coll->insert(array('_id' => 1));
$coll->insert(array('_id' => 2));

$collOut = $mc->selectCollection(dbname(), collname(__FILE__) . '-out');
$collOut->drop();

$coll->setReadPreference(
    MongoClient::RP_PRIMARY_PREFERRED,
    array(array('dc' => 'sf'))
);

echo "Testing with aggregate()\n";

$coll->aggregate(array(
    array('$match' => array('_id' => 2)),
    array('$out' => $collOut->getName())
));

printf("Output collection count: %d\n", $collOut->count());

echo "\nTesting with aggregateCursor()\n";

// The command is not executed until we rewind the cursor
$cursor = $coll->aggregateCursor(array(
    array('$match' => array('_id' => 2)),
    array('$out' => $collOut->getName())
))->rewind();

printf("Output collection count: %d\n", $collOut->count());

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Testing with aggregate()
Forcing aggregate with $out to run on primary

Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
Output collection count: 1

Testing with aggregateCursor()
Forcing aggregate with $out to run on primary

Warning: MongoCollection::aggregateCursor(): Forcing aggregate with $out to run on primary in %s on line %d
Output collection count: 1
===DONE===
