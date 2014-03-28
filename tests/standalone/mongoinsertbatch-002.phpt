--TEST--
MongoInsertBatch: Batch Splitting, overflowing maxBsonSize
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

/* Space for 4 documents */
$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($retval);

$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($retval);

$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($retval);

$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($retval);

/* This one will overflow */
$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($retval);

$retval = $batch->execute(array("w" => 1));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
/* We should get two indexes now, n=4 and n=1 */
var_dump($retval);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
int(1)
array(1) {
  [0]=>
  array(2) {
    ["count"]=>
    int(1)
    ["size"]=>
    int(4193420)
  }
}
bool(true)
int(2)
array(1) {
  [0]=>
  array(2) {
    ["count"]=>
    int(2)
    ["size"]=>
    int(8386739)
  }
}
bool(true)
int(3)
array(1) {
  [0]=>
  array(2) {
    ["count"]=>
    int(3)
    ["size"]=>
    int(12580058)
  }
}
bool(true)
int(4)
array(1) {
  [0]=>
  array(2) {
    ["count"]=>
    int(4)
    ["size"]=>
    int(16773377)
  }
}
bool(true)
int(5)
array(2) {
  [0]=>
  array(2) {
    ["count"]=>
    int(4)
    ["size"]=>
    int(16773377)
  }
  [1]=>
  array(2) {
    ["count"]=>
    int(1)
    ["size"]=>
    int(4193420)
  }
}
bool(true)
int(0)
array(0) {
}
array(2) {
  ["nInserted"]=>
  int(5)
  ["ok"]=>
  bool(true)
}
===DONE===
