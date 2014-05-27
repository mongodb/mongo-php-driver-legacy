--TEST--
MongoInsertBatch::add() accepts object argument
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

$insert = array('_id' => 1, 'foo' => 'bar');

$batch = new MongoInsertBatch($collection);
$batch->add((object) $insert);
$batch->execute();

var_dump(iterator_to_array($collection->find()));

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(1) {
  [1]=>
  array(2) {
    ["_id"]=>
    int(1)
    ["foo"]=>
    string(3) "bar"
  }
}
===DONE===
