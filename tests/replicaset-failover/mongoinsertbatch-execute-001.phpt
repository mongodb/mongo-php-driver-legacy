--TEST--
MongoInsertBatch::execute() during failover
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function get_primary_maxWriteBatchSize(MongoClient $mc) {
    foreach ($mc->getConnections() as $c) {
        if ($c['connection']['connection_type'] == 2) {
            return $c['connection']['max_write_batch_size'];
        }
    }

    throw new Exception('Cannot get maxWriteBatchSize; no primary found!');
}

$server = new MongoShellServer;
$rs = $server->getReplicasetConfig();

function log_reply() {
    global $server;
    static $i = 0;

    printf("Received reply from batch: %d\n", ++$i);

    if ($i === 1) {
        echo "Killing master\n";
        $server->killMaster();
        echo "Master killed\n";
    }
}

$ctx = stream_context_create(array("mongodb" => array(
    "log_reply" => "log_reply",
)));

$mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"]), array("context" => $ctx));
$collection = $mc->selectCollection(dbname(), collname(__FILE__));

$collection->drop();

$batch = new MongoInsertBatch($collection);

$maxWriteBatchSize = get_primary_maxWriteBatchSize($mc);

for ($i = 0; $i < $maxWriteBatchSize + 1; $i++) {
    $batch->add(array('x' => $i));
}

try {
    $batch->execute(array('w' => 1));
    echo "Write succeeded without a primary!\n";
} catch(MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--CLEAN--
<?php require_once "tests/utils/fix-master.inc"; ?>
--EXPECTF--
Received reply from batch: 1
Killing master
Master killed
string(%d) "%s:%d: Remote server has closed the connection"
int(32)
