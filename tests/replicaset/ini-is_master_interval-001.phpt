--TEST--
"mongo.is_master_interval" INI option
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
--INI--
mongo.is_master_interval=95
--FILE--
<?php

function handleNotice($code, $message) {
    if (preg_match('/ismaster:.*left: (\d+)/', $message, $m)) {
        echo "LEFT: {$m[1]}\n";
    }
}

set_error_handler('handleNotice', E_NOTICE);

MongoLog::setModule(MongoLog::CON);
MongoLog::setLevel(MongoLog::FINE);

require_once "tests/utils/server.inc";

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"]));

$coll = $mc->selectCollection(dbname(), 'mongo-is_master_interval');
$coll->drop();

echo "---\n";

ini_set('mongo.is_master_interval', 75);
$coll->insert(array('_id' => 125, 'x' => 'foo'));

MongoLog::setModule(MongoLog::NONE);
MongoLog::setLevel(MongoLog::NONE);
?>
--EXPECTF--
LEFT: 9%d
LEFT: 9%d
LEFT: 9%d
LEFT: 9%d
LEFT: 9%d
---
LEFT: 7%d
LEFT: 7%d
LEFT: 7%d
LEFT: 7%d
LEFT: 7%d
