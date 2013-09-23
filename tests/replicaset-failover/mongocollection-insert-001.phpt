--TEST--
MongoClient::insert() during failover (1)
--SKIPIF--
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$server = new MongoShellServer;
$rs = $server->getReplicasetConfig();
$mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"]));
$coll = $mc->selectCollection("failover", "test1");

for($i = 0; $i < 10; $i++) {
    /* Just to make sure master is working fine */
    try {
        $coll->insert(array("doc" => $i));
    } catch(Exception $e) {
        echo "This was not supposed to happen!!!\n";
        var_dump(get_class($e), $e->getMessage(), $e->getCode());
    }
}
$i++;
echo "Killing master\n";
$server->killMaster();
echo "Master killed\n";
try {
    $coll->insert(array("doc" => $i));
    echo "That query should have failed\n";
} catch(MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}
for(; $i < 20; $i++) {
    try {
        $coll->insert(array("doc" => $i));
        echo "That query probably should have failed\n";
    } catch(MongoCursorException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }
}
for($i=0;$i<60; $i++) {
    try {
        $coll->insert(array("doc" => $i));
        echo "Found master again\n";
        break;
    } catch(MongoCursorException $e) {
    }
}
?>
--CLEAN--
<?php require_once "tests/utils/fix-master.inc"; ?>
--EXPECTF--
Killing master
Master killed
string(%d) "%s:%d: %s"
int(32)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
string(51) "Couldn't get connection: No candidate servers found"
int(16)
