--TEST--
MongoUpdateBatch::execute() Updating one and multiple documents in separate batches
--DESCRIPTION--
This tests batches consisting of single update operations.
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

printf("Updating one document (multi unspecified) in a single batch\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('foo' => 'abc')),
);

$batch = new MongoUpdateBatch($collection);
var_dump($batch->add($update));
$res = $batch->execute(array('w' => 1));
dump_these_keys($res, array('nMatched', 'nModified', 'nUpserted', 'ok'));

printf("Updating one document (multi = false) in a single batch\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('foo' => 'def')),
  'multi' => false,
);

$batch = new MongoUpdateBatch($collection);
var_dump($batch->add($update));
$res = $batch->execute(array('w' => 1));
dump_these_keys($res, array('nMatched', 'nModified', 'nUpserted', 'ok'));

printf("Updating multiple documents (multi = true) in a single batch\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('foo' => 'ghi')),
  'multi' => true,
);

$batch = new MongoUpdateBatch($collection);
var_dump($batch->add($update));
$res = $batch->execute(array('w' => 1));
dump_these_keys($res, array('nMatched', 'nModified', 'nUpserted', 'ok'));

printf("Upserting one document (with _id) in a single batch\n");

$update = array(
  'q' => array('_id' => 5),
  'u' => array('$set' => array('x' => 2)),
  'upsert' => true,
);

$batch = new MongoUpdateBatch($collection);
var_dump($batch->add($update));
$res = $batch->execute(array('w' => 1));
dump_these_keys($res, array('upserted', 'nMatched', 'nModified', 'nUpserted', 'ok'));

printf("Upserting one document (_id unspecified) in a single batch\n");

$update = array(
  'q' => array('foo' => 'bar'),
  'u' => array('$set' => array('x' => 1)),
  'upsert' => true,
);

$batch = new MongoUpdateBatch($collection);
var_dump($batch->add($update));
$res = $batch->execute(array('w' => 1));
dump_these_keys($res, array('upserted', 'nMatched', 'nModified', 'nUpserted', 'ok'));

printf("Total batches sent to server: %d\n", $numBatches);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Updating one document (multi unspecified) in a single batch
bool(true)
array(4) {
  ["nMatched"]=>
  int(1)
  ["nModified"]=>
  int(1)
  ["nUpserted"]=>
  int(0)
  ["ok"]=>
  bool(true)
}
Updating one document (multi = false) in a single batch
bool(true)
array(4) {
  ["nMatched"]=>
  int(1)
  ["nModified"]=>
  int(1)
  ["nUpserted"]=>
  int(0)
  ["ok"]=>
  bool(true)
}
Updating multiple documents (multi = true) in a single batch
bool(true)
array(4) {
  ["nMatched"]=>
  int(2)
  ["nModified"]=>
  int(2)
  ["nUpserted"]=>
  int(0)
  ["ok"]=>
  bool(true)
}
Upserting one document (with _id) in a single batch
bool(true)
array(5) {
  ["upserted"]=>
  array(1) {
    [0]=>
    array(2) {
      ["index"]=>
      int(0)
      ["_id"]=>
      int(5)
    }
  }
  ["nMatched"]=>
  int(0)
  ["nModified"]=>
  int(0)
  ["nUpserted"]=>
  int(1)
  ["ok"]=>
  bool(true)
}
Upserting one document (_id unspecified) in a single batch
bool(true)
array(5) {
  ["upserted"]=>
  array(1) {
    [0]=>
    array(2) {
      ["index"]=>
      int(0)
      ["_id"]=>
      object(MongoId)#%d (1) {
        ["$id"]=>
        string(24) "%s"
      }
    }
  }
  ["nMatched"]=>
  int(0)
  ["nModified"]=>
  int(0)
  ["nUpserted"]=>
  int(1)
  ["ok"]=>
  bool(true)
}
Total batches sent to server: 5
===DONE===
