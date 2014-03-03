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

$collection = $mc->selectCollection("test", "insertbatch");
$collection->drop();


$batch = new MongoInsertBatch($collection);

/* Space for 4 documents */
$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($retval);

$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($retval);

$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($retval);

$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($retval);

/* This one will overflow */
$content = str_repeat('x', 4 * 1024 * 1024 - 1024);
$retval = $batch->add(array("content" => $content));
var_dump($retval);

$retval = $batch->execute(array("w" => 1));
/* We should get two indexes now, n=4 and n=1 */
var_dump($retval);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
array(2) {
  ["nInserted"]=>
  int(5)
  ["ok"]=>
  bool(true)
}
===DONE===
