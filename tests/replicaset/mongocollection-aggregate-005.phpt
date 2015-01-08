--TEST--
MongoCollection::aggregate() with $out should force primary
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

$coll->insert(array('_id' => 1));
$coll->insert(array('_id' => 2));

/* If the pipeline ends with $out, aggregate() should force a primary read pref
 * over all others. This includes primaryPreferred, since it's possible, albeit
 * unlikely, for the command to hit a secondary.
 */
$readPrefs = array(
    MongoClient::RP_PRIMARY_PREFERRED,
    MongoClient::RP_SECONDARY,
    MongoClient::RP_SECONDARY_PREFERRED,
    MongoClient::RP_NEAREST,
);

foreach ($readPrefs as $readPref) {
    $collOut->drop();

    $coll->setReadPreference($readPref);

    $rp = $coll->getReadPreference();
    printf("Collection read preference: %s\n", $rp['type']);

    $coll->aggregate(array(
        array('$match' => array('_id' => 2)),
        array('$out' => $collOut->getName())
    ));

    printf("Output collection count: %d\n", $collOut->count());

    $rp = $coll->getReadPreference();
    printf("Collection read preference: %s\n", $rp['type']);

    echo "\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Collection read preference: primaryPreferred
Forcing aggregate with $out to run on primary

Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
Output collection count: 1
Collection read preference: primaryPreferred

Collection read preference: secondary
Forcing aggregate with $out to run on primary

Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
Output collection count: 1
Collection read preference: secondary

Collection read preference: secondaryPreferred
Forcing aggregate with $out to run on primary

Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
Output collection count: 1
Collection read preference: secondaryPreferred

Collection read preference: nearest
Forcing aggregate with $out to run on primary

Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
Output collection count: 1
Collection read preference: nearest

===DONE===
