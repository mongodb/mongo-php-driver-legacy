--TEST--
MongoWriteBatch: test nStuff counting
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
$batch->add(array("a" => 1));
$retval = $batch->execute(array("w" => 1));
var_dump($retval);

$batch = new MongoUpdateBatch($collection);
$batch->add(array("q" => array("a" => 1), "u" => array('$set' => array('b' => 1)), "multi" => 1));
$batch->add(array("q" => array("a" => 2), "u" => array('$set' => array('b' => 2)), "multi" => 1, "upsert" => 1));
$retval = $batch->execute(array("w" => 1));
var_dump($retval);

$batch = new MongoInsertBatch($collection);
$batch->add(array("a" => 3));
$retval = $batch->execute(array("w" => 1));
var_dump($retval);

$batch = new MongoDeleteBatch($collection);
$batch->add(array("q" => array("a" => 3), "limit" => 1));
$retval = $batch->execute(array("w" => 1));
var_dump($retval);



?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(2) {
  ["nInserted"]=>
  int(1)
  ["ok"]=>
  bool(true)
}
array(5) {
  ["upserted"]=>
  array(1) {
    [0]=>
    array(2) {
      ["index"]=>
      int(1)
      ["_id"]=>
      object(MongoId)#%d (1) {
        ["$id"]=>
        string(24) "%s"
      }
    }
  }
  ["nMatched"]=>
  int(1)
  ["nModified"]=>
  int(1)
  ["nUpserted"]=>
  int(1)
  ["ok"]=>
  bool(true)
}
array(2) {
  ["nInserted"]=>
  int(1)
  ["ok"]=>
  bool(true)
}
array(2) {
  ["nRemoved"]=>
  int(1)
  ["ok"]=>
  bool(true)
}
===DONE===
