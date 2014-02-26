--TEST--
MongoInsertBatch: Batch Splitting, overflowing maxWriteBatchSize
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

for ($i=1; $i<=3001; $i++) {
    $retval = $batch->add(array("document" => $i));
}


echo "Executing the batch now\n";
$retval = $batch->execute(array("w" => 1));
var_dump($retval);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Executing the batch now
array(2) {
  ["nInserted"]=>
  int(3001)
  ["ok"]=>
  bool(true)
}
===DONE===
