--TEST--
MongoCommandCursor::setReadPreference() can change read preference between multiple executions
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$coll = $mc->selectCollection(dbname(), collname(__FILE__));
$coll->drop();

$coll->insert(array('_id' => 1), array('w' => 4)); // there are four data carrying nodes

$command = array(
    'aggregate' => $coll->getName(),
    'pipeline' => array(array('$limit' => 1)),
    'cursor' => new stdClass(),
);

$cc = new MongoCommandCursor($mc, (string) $coll, $command);
$cc->setReadPreference(MongoClient::RP_SECONDARY);

$cc->rewind();
$info = $cc->info();
echo $info['connection_type_desc'], "\n";

$cc = new MongoCommandCursor($mc, (string) $coll, $command);
$cc->setReadPreference(MongoClient::RP_PRIMARY);

$cc->rewind();
$info = $cc->info();
echo $info['connection_type_desc'], "\n";

?>
===DONE===
--EXPECT--
SECONDARY
PRIMARY
===DONE===
