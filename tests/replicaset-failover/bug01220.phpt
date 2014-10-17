--TEST--
Test for PHP-1220: Cannot find primary (not in seed list) after election
--SKIPIF--
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--INI--
mongo.is_master_interval=1
--FILE--
<?php
require_once "tests/utils/server.inc";

function getPrimaryNode(MongoShellServer $server)
{
    $config = $server->getMaster();
    return $config[2];
}

function getASecondaryNode(MongoShellServer $server)
{
    $config = $server->getMaster();
    $secondaries = array_diff($config[3], array($config[2]));
    return current($secondaries);
}

$server = new MongoShellServer();
$primary = getPrimaryNode($server);
$secondary = getASecondaryNode($server);

/* Limit the seed list to the current primary and frozen secondary */
$dsn = $primary . ',' . $secondary;
$rs = $server->getReplicasetConfig();

/* Setup data on primary and replicate to all 4 nodes (arbiter is the 5th) */
$mc = new MongoClient($dsn, array('replicaSet' => $rs['rsname']));
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();
$result = $collection->insert(array('_id' => 1), array('w' => 4));
printf("Inserted data on primary: %s\n", empty($result['ok']) ? 'no' : 'yes');

$result = $collection->findOne(array('_id' => 1));
printf("Found data on primary: %s\n", empty($result) ? 'no' : 'yes');

$collection->setReadPreference(MongoClient::RP_SECONDARY);
$result = $collection->findOne(array('_id' => 1));
printf("Found data on a secondary: %s\n", empty($result) ? 'no' : 'yes');

unset($collection);
unset($mc);

$freezeAt = microtime(true);

/* Freeze a secondary to make it ineligible for election */
$mc = new MongoClient($secondary, array('readPreference' => MongoClient::RP_SECONDARY));
echo 'Freezing one secondary...';
$result = $mc->admin->command(array('replSetFreeze' => 30));
printf(" %s\n", empty($result['ok']) ? 'failed' : 'done');

unset($mc);

/* Connect to the primary and force it to step down */
echo 'Stepping down primary...';
try {
    $mc = new MongoClient($primary);
    $result = $mc->admin->command(array('replSetStepDown' => 15, 'force' => true));
    echo " failed\n";
    var_dump($result);
} catch (MongoCursorException $e) {
    echo " done\n";
}

unset($mc);

echo 'Waiting until new primary is elected...';
$server->restartMaster();
echo " done\n";

printf("Election took %.1f seconds\n", microtime(true) - $freezeAt);
printf("New primary differs from old primary: %s\n", $primary == getPrimaryNode($server) ? 'no' : 'yes');

/* Connect with the old seed list and ensure new primary is discovered */
$mc = new MongoClient($dsn, array('replicaSet' => $rs['rsname']));
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$result = $collection->findOne(array('_id' => 1));
printf("Found data on new primary: %s\n", empty($result) ? 'no' : 'yes');

unset($collection);
unset($mc);

$freezeRemaining = ceil(max(15 - (microtime(true) - $freezeAt), 0));
printf("Sleeping %d seconds until old primary freeze time expires\n", $freezeRemaining);
sleep($freezeRemaining);

$revertAt = microtime(true);

/* Connect to the new primary and force it to step down */
echo 'Stepping down new primary...';
try {
    $mc = new MongoClient(getPrimaryNode($server));
    $result = $mc->admin->command(array('replSetStepDown' => 15, 'force' => true));
    echo " failed\n";
    var_dump($result);
} catch (MongoCursorException $e) {
    echo " done\n";
}

unset($mc);

echo 'Waiting until old primary is re-elected...';
$server->restartMaster();
echo " done\n";

printf("Election took %.1f seconds\n", microtime(true) - $revertAt);
printf("Old primary has been re-elected: %s\n", $primary == getPrimaryNode($server) ? 'yes' : 'no');

$freezeRemaining = ceil(max(30 - (microtime(true) - $freezeAt), 0));
printf("Sleeping %d seconds until secondary freeze time expires\n", $freezeRemaining);
sleep($freezeRemaining);

?>
===DONE===
<?php exit(0); ?>
--CLEAN--
<?php require_once "tests/utils/fix-master.inc"; ?>
--EXPECTF--
Inserted data on primary: yes
Found data on primary: yes
Found data on a secondary: yes
Freezing one secondary... done
Stepping down primary... done
Waiting until new primary is elected... done
Election took %f seconds
New primary differs from old primary: yes
Found data on new primary: yes
Sleeping %d seconds until old primary freeze time expires
Stepping down new primary... done
Waiting until old primary is re-elected... done
Election took %f seconds
Old primary has been re-elected: yes
Sleeping %d seconds until secondary freeze time expires
===DONE===
