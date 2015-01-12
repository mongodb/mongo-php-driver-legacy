--TEST--
MongoCommandCursor::setReadPreference() does not change server during iteration
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";

function log_getmore($server, $info) {
    echo "Issuing getmore\n";
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_getmore' => 'log_getmore',
    ),
));

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']), array('context' => $ctx));

$coll = $mc->selectCollection(dbname(), collname(__FILE__));
$coll->drop();

$coll->insert(array('_id' => 1));
$coll->insert(array('_id' => 2), array('w' => 4)); // there are four data carrying nodes

$command = array(
    'aggregate' => $coll->getName(),
    'pipeline' => array(array('$limit' => 2)),
    'cursor' => array('batchSize' => 1),
);

$cc = new MongoCommandCursor($mc, (string) $coll, $command);

// Starting with a secondary read pref, ensure the server does not change during iteration

$cc->setReadPreference(MongoClient::RP_SECONDARY);

$cc->rewind();
$info = $cc->info();
echo $info['connection_type_desc'], "\n";

$cc->setReadPreference(MongoClient::RP_PRIMARY);

$cc->next();
$info = $cc->info();
echo $info['connection_type_desc'], "\n";

// After rewinding, the new read preference should be used

$cc->rewind();
$info = $cc->info();
echo $info['connection_type_desc'], "\n";

$cc->setReadPreference(MongoClient::RP_SECONDARY);

$cc->next();
$info = $cc->info();
echo $info['connection_type_desc'], "\n";

?>
===DONE===
--EXPECT--
SECONDARY
Issuing getmore
SECONDARY
PRIMARY
Issuing getmore
PRIMARY
===DONE===
