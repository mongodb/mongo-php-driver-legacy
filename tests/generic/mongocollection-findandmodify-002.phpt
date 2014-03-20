--TEST--
MongoCollection::findAndModify() with maxTimeMS option
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();
$collection->insert(array('x' => 1));

configureFailPoint($mc, 'maxTimeAlwaysTimeOut', 1);

echo "\nfindAndModify without maxTimeMS option\n";

$document = $collection->findAndModify(
  array('x' => 1),
  array('$set' => array('x' => 2)),
  array(),
  array('new' => true)
);

printf("Set x to %d\n", $document['x']);

echo "\nfindAndModify with maxTimeMS option\n";

try {
  $document = $collection->findAndModify(
    array('x' => 2),
    array('$set' => array('x' => 3)),
    array(),
    array('new' => true, 'maxTimeMS' => 100)
  );
  printf("Set x to %d\n", $document['x']);
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Configured maxTimeAlwaysTimeOut fail point

findAndModify without maxTimeMS option
Set x to 2

findAndModify with maxTimeMS option
exception class: MongoExecutionTimeoutException
exception message: %s:%d: operation exceeded time limit
exception code: 50
===DONE===
