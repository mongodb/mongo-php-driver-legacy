--TEST--
MongoInsertBatch: Reusing MongoWriteBatch for multiple batches
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection("test", "insertbatch");
$collection->drop();


$batch = new MongoInsertBatch($collection);
$batch->add(array());
$exeretval = $batch->execute(array("w" => 1));
var_dump($exeretval);

$batch->add(array());
$exeretval = $batch->execute(array("w" => 1));
var_dump($exeretval);
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(1) {
  [0]=>
  array(2) {
    ["ok"]=>
    bool(true)
    ["n"]=>
    int(1)
  }
}
array(1) {
  [0]=>
  array(2) {
    ["ok"]=>
    bool(true)
    ["n"]=>
    int(1)
  }
}
===DONE===
