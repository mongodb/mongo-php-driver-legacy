--TEST--
Test for PHP-751: When a secondary goes into recovery mode, we should disconnect from it
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$server = new MongoShellServer;
$rs = $server->getReplicasetConfig();

function log_server_type($server) {
    printf("Server type: %s (%d)\n", $server["type"] == 2 ? "PRIMARY" : ($server["type"] == 4 ? "SECONDARY" : "UNKNOWN"), $server["type"]);
}

$ctx = stream_context_create(array("mongodb" => array(
    "log_query" => "log_server_type",
    "log_cmd_insert" => "log_server_type",
    "log_cmd_delete" => "log_server_type",
)));

$mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"], "readPreference" => MongoClient::RP_SECONDARY_PREFERRED), array("context" => $ctx));
$i = "random";
$mc->selectDb(dbname())->recoverymode->drop();
$mc->selectDb(dbname())->recoverymode->insert(array("doc" => $i, "w" => "majority"));

// Lower the ismaster interval to make sure we pick up on the change
ini_set("mongo.is_master_interval", 1);

echo "Putting all secondaries into recovery mode\n";
$server->setMaintenanceForSecondaries(true);
sleep(3);

echo "We should have detected that the servers are in maintenence mode now\n";
echo "This should hit the primary as all secondaries are in recovery\n";
for($i=0; $i < 10; $i++) {
    try {
        $mc->selectDb(dbname())->recoverymode->findOne(array("doc" => $i));
    } catch(MongoCursorException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }
}

echo "Enabling all secondaries again\n";
$server->setMaintenanceForSecondaries(false);
sleep(3);

echo "This should hit secondaries as they are no longer in recovery\n";
for($i=0; $i < 10; $i++) {
    try {
        $mc->selectDb(dbname())->recoverymode->findOne(array("doc" => $i));
    } catch(MongoCursorException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }
}

// Increase the interval so we don't notice the change until we hit the servers
ini_set("mongo.is_master_interval", 50);
echo "Putting all secondaries into recovery mode\n";
$server->setMaintenanceForSecondaries(true);

echo "These should throw exception as we haven't detected that the server is in recovery mode yet\n";
/* Once per secondary */
for($i=0; $i < 3; $i++) {
    try {
        $mc->selectDb(dbname())->recoverymode->findOne(array("doc" => $i));
    } catch(MongoCursorException $e) {
        var_dump(get_class($e), $e->getMessage(), $e->getCode());
    }
}

echo "This should hit the primary as all secondaries are in recovery\n";
for($i=0; $i < 10; $i++) {
    try {
        $mc->selectDb(dbname())->recoverymode->findOne(array("doc" => $i));
    } catch(MongoCursorException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }
}

echo "Enabling all secondaries again in a CLEAN section\n";

?>
--CLEAN--
<?php require_once "tests/utils/fix-secondaries.inc"; ?>
--EXPECTF--
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Putting all secondaries into recovery mode
We should have detected that the servers are in maintenence mode now
This should hit the primary as all secondaries are in recovery
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Enabling all secondaries again
This should hit secondaries as they are no longer in recovery
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Server type: SECONDARY (4)
Putting all secondaries into recovery mode
These should throw exception as we haven't detected that the server is in recovery mode yet
Server type: SECONDARY (4)
string(20) "MongoCursorException"
string(%d) "%s: not master or secondary; cannot currently read from this replSet member"
int(13436)
Server type: SECONDARY (4)
string(20) "MongoCursorException"
string(%d) "%s: not master or secondary; cannot currently read from this replSet member"
int(13436)
Server type: SECONDARY (4)
string(20) "MongoCursorException"
string(%d) "%s: not master or secondary; cannot currently read from this replSet member"
int(13436)
This should hit the primary as all secondaries are in recovery
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Server type: PRIMARY (2)
Enabling all secondaries again in a CLEAN section

