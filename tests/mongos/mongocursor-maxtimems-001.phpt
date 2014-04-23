--TEST--
MongoCursor::maxTimeMS()
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php $needs = "2.5.3"; require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cfg = MongoShellServer::getShardInfo();
$mc = new MongoClient($cfg[0]);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

for ($i = 0; $i < 10000; $i++) {
    $collection->insert(array('foo' => $i));
}

$cursor = $collection->find();
$cursor->maxTimeMS(1);

try {
    iterator_to_array($cursor);
} catch (MongoExecutionTimeoutException $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}
?>
===DONE===
--EXPECTF--
exception class: MongoExecutionTimeoutException
exception message: %s:%d: operation exceeded time limit
exception code: 50
===DONE===
