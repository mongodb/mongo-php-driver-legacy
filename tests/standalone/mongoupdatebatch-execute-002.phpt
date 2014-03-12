--TEST--
MongoUpdateBatch::execute() Updating one and multiple documents in the same batch
--DESCRIPTION--
This tests batches consisting of multiple update operations. That means that
update result merging is also implicitly tested.
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

$batch = new MongoUpdateBatch($collection);

printf("Updating one document (multi unspecified) as batch op #1\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('foo' => 'abc')),
);

var_dump($batch->add($update));

printf("Updating one document (multi = false) as batch op #2\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('foo' => 'def')),
  'multi' => false,
);

var_dump($batch->add($update));

printf("Updating multiple documents (multi = true) as batch op #3\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('foo' => 'ghi')),
  'multi' => true,
);

var_dump($batch->add($update));

printf("Upserting one document (with _id) as batch op #4\n");

$update = array(
  'q' => array('_id' => 5),
  'u' => array('$set' => array('x' => 2)),
  'upsert' => true,
);

var_dump($batch->add($update));

printf("Upserting one document (_id unspecified) as batch op #5\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('x' => 1)),
  'upsert' => true,
);

var_dump($batch->add($update));

$res = $batch->execute(array('w' => 1));
dump_these_keys($res, array('upserted', 'nMatched', 'nModified', 'nUpserted', 'ok'));

printf("Total batches sent to server: %d\n", $numBatches);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Updating one document (multi unspecified) as batch op #1
bool(true)
Updating one document (multi = false) as batch op #2
bool(true)
Updating multiple documents (multi = true) as batch op #3
bool(true)
Upserting one document (with _id) as batch op #4
bool(true)
Upserting one document (_id unspecified) as batch op #5
bool(true)
array(5) {
  ["upserted"]=>
  array(2) {
    [0]=>
    array(2) {
      ["index"]=>
      int(3)
      ["_id"]=>
      int(5)
    }
    [1]=>
    array(2) {
      ["index"]=>
      int(4)
      ["_id"]=>
      object(MongoId)#%d (1) {
        ["$id"]=>
        string(24) "%s"
      }
    }
  }
  ["nMatched"]=>
  int(4)
  ["nModified"]=>
  int(4)
  ["nUpserted"]=>
  int(2)
  ["ok"]=>
  bool(true)
}
Total batches sent to server: 1
===DONE===
