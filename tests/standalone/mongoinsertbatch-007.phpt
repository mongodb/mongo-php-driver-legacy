--TEST--
MongoWriteBatch: PHP-1019: Do not allow executing an empty batch
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();


$batch = new MongoInsertBatch($collection);
try {
    $retval = $batch->execute(array("w" => 1));
    var_dump($retval);
    echo "FAILED\n!";
} catch(MongoException $e) {
    echo $e->getMessage(), "\n";
}

try {
    $retval = $collection->batchInsert(array());
    var_dump($retval);
    echo "FAILED\n!";
} catch(MongoException $e) {
    echo $e->getMessage(), "\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
No write ops were included in the batch
No write ops were included in the batch
===DONE===
