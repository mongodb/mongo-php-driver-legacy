--TEST--
MongoInsertBatch: Batch Splitting, overflowing maxWriteBatchSize (32bit)
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();


$batch = new MongoInsertBatch($collection);

for ($i=1; $i<=3001; $i++) {
    $retval = $batch->add(array("document" => $i));
}


echo "Executing the batch now\n";
var_dump($batch->getItemCount(), $batch->getBatchInfo());
$retval = $batch->execute(array("w" => 1));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($retval);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Executing the batch now
int(3001)
array(4) {
  [0]=>
  array(2) {
    ["count"]=>
    int(1000)
    ["size"]=>
    int(40997)
  }
  [1]=>
  array(2) {
    ["count"]=>
    int(1000)
    ["size"]=>
    int(40997)
  }
  [2]=>
  array(2) {
    ["count"]=>
    int(1000)
    ["size"]=>
    int(40997)
  }
  [3]=>
  array(2) {
    ["count"]=>
    int(1)
    ["size"]=>
    int(146)
  }
}
int(0)
array(0) {
}
array(2) {
  ["nInserted"]=>
  int(3001)
  ["ok"]=>
  bool(true)
}
===DONE===
