--TEST--
MongoDeleteBatch::execute() Deleting one and multiple documents in the same batch
--DESCRIPTION--
This tests batches consisting of multiple delete operations. Merging results
from delete operations is also implicitly tested.
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$numBatches = 0;

function log_write_batch($server, $write_options, $batch, $protocol_options) {
  global $numBatches;

  $numBatches += 1;
}

$ctx = stream_context_create(array(
  'mongodb' => array('log_write_batch' => 'log_write_batch'),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

$collection->insert(array('_id' => 1, 'foo' => 'bar'));
$collection->insert(array('_id' => 2, 'foo' => 'bar'));
$collection->insert(array('_id' => 3, 'foo' => 'bar'));
$collection->insert(array('_id' => 4, 'foo' => 'bar'));

$batch = new MongoDeleteBatch($collection);

printf("Deleting one document (limit = 1) as batch op #1\n");

$delete = array(
  'q' => array('foo' => 'bar'),
  'limit' => 1,
);

var_dump($batch->add($delete));

printf("Deleting multiple documents (limit = 0) as batch op #2\n");

$delete = array(
  'q' => array('foo' => 'bar'),
  'limit' => 0,
);

var_dump($batch->add($delete));

$res = $batch->execute(array('w' => 1));
dump_these_keys($res, array('nRemoved', 'ok'));

printf("Total batches sent to server: %d\n", $numBatches);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Deleting one document (limit = 1) as batch op #1
bool(true)
Deleting multiple documents (limit = 0) as batch op #2
bool(true)
array(2) {
  ["nRemoved"]=>
  int(4)
  ["ok"]=>
  bool(true)
}
Total batches sent to server: 1
===DONE===
