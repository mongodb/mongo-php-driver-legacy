--TEST--
MongoClient::__construct()#002 during failover
--SKIPIF--
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$server = new MongoShellServer;
$rs = $server->getReplicasetConfig();

function log_query($server, $query, $cursor_options) {
    var_dump($server, $query, $cursor_options);
}
$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_query" => "log_query",
        )
    )
);
$mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"]), array("context" => $ctx));

$coll = $mc->selectCollection("ctorfailover", "test1");
$data = array("x" => "The world is not enough");
$coll->insert($data);
$id = $data["_id"];

echo "About to kill master\n";
$server->killMaster();
echo "Master killed\n";


$t = time();
try {
    echo "Attempting insert\n";
    $coll->insert($data);
    echo "failed, somehow managed to insert when no primary was found\n";
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}

echo "Doing secondary read\n";
$data = $coll->setReadPreference(MongoClient::RP_SECONDARY); 
$coll->findOne(array("_id" => $id));

echo "Doing primary read, should fail since we don't have primary\n";
try {
    $coll->setReadPreference(MongoClient::RP_PRIMARY); 
    $coll->findOne(array("_id" => $id));
    echo "Did a primary read without a primary?!\n";
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}
// Since the cleanup is part of this test it can take a while.. this section 
// should definitily not take more then 3secs
// The only reason we have this here though is to verify we aren't wasting 
// time in attempting to reconnect to master and blocking
var_dump(time()-$t > 3);
?>
--CLEAN--
<?php require_once "tests/utils/fix-master.inc"; ?>
--EXPECTF--
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;REPLICASET;X;%d"
  ["type"]=>
  int(2)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(48000000)
  ["request_id"]=>
  int(%d)
}
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(5) {
  ["request_id"]=>
  int(1)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(0)
  ["cursor_id"]=>
  int(0)
}
About to kill master
Master killed
Attempting insert
array(5) {
  ["hash"]=>
  string(%s) "%s:%d;REPLICASET;X;%d"
  ["type"]=>
  int(2)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(48000000)
  ["request_id"]=>
  int(%d)
}
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(5) {
  ["request_id"]=>
  int(3)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(0)
  ["cursor_id"]=>
  int(0)
}
string(20) "MongoCursorException"
string(58) "%s:%d: Remote server has closed the connection"
int(3)
Doing secondary read
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;REPLICASET;X;%d"
  ["type"]=>
  int(4)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(48000000)
  ["request_id"]=>
  int(%d)
}
array(1) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
array(5) {
  ["request_id"]=>
  int(4)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(4)
  ["cursor_id"]=>
  int(0)
}
Doing primary read, should fail since we don't have primary
string(24) "MongoConnectionException"
string(26) "No candidate servers found"
int(71)
bool(false)
