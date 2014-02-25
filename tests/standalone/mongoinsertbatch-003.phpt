--TEST--
MongoInsertBatch Batch Splitting, overflowing maxWriteBatchSize
--SKIPIF--
<?php $needs = "2.5.5"; ?>
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
array(4) {
  [0]=>
  array(2) {
    ["ok"]=>
    bool(true)
    ["n"]=>
    int(1000)
  }
  [1]=>
  array(2) {
    ["ok"]=>
    bool(true)
    ["n"]=>
    int(1000)
  }
  [2]=>
  array(2) {
    ["ok"]=>
    bool(true)
    ["n"]=>
    int(1000)
  }
  [3]=>
  array(2) {
    ["ok"]=>
    bool(true)
    ["n"]=>
    int(1)
  }
}
===DONE===
