--TEST--
MongoWriteBatch: Throw exception on invalid wire version
--SKIPIF--
<?php $needs = "2.5.0"; $needsOp = "lt"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection("test", "insertbatch");
$collection->drop();


try {
    $batch = new MongoInsertBatch($collection);
} catch(MongoProtocolException $e) {
    echo $e->getMessage(), "\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Current primary does not have a Write API support
===DONE===
