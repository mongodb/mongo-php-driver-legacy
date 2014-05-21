--TEST--
MongoDeleteBatch::add() accepts object argument
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

$collection->insert(array('_id' => 1, 'foo' => 'bar'));

$delete = array(
  'q' => array('foo' => 'bar'),
  'limit' => 0,
);

$batch = new MongoDeleteBatch($collection);
$batch->add((object) $delete);
$batch->execute();

var_dump(iterator_to_array($collection->find()));

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(0) {
}
===DONE===
