--TEST--
MongoClient::__construct() during failover (1)
--SKIPIF--
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$server = new MongoShellServer;
$rs = $server->getReplicasetConfig();

/* Just to have some data to read */
$mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"]));
$coll = $mc->selectCollection("ctorfailover", "test1");
$data = array("x" => "The world is not enough");
$coll->insert($data);
$id = $data["_id"];
$coll->insert(array("something" => "else"), array("w" => 2));
$coll = $mc = null;

echo "About to kill master\n";
$server->killMaster();
echo "Master killed\n";


for($i = 0; $i<10; $i++) {
    try {
        $mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"], "readPreference" => MongoClient::RP_SECONDARY));
        $coll = $mc->selectCollection("ctorfailover", "test1");
        $data = $coll->findOne(array("_id" => $id));
        var_dump($data["x"]);
    } catch(Exception $e) {
        var_dump(get_class($e), $e->getMessage(), $e->getCode());
    }
}
try {
    $mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"]));
    $coll = $mc->selectCollection("ctorfailover", "test1");
    $data = $coll->findOne(array("_id" => $id));
    echo "Primary is up for some reason, that shouldn't have happened!\n";
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}
?>
--CLEAN--
<?php require_once "tests/utils/fix-master.inc"; ?>
--EXPECTF--
About to kill master
Master killed
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(23) "The world is not enough"
string(20) "MongoCursorException"
string(%d) "%s:%d: Remote server has closed the connection"
int(32)

