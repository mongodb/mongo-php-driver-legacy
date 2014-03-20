--TEST--
MongoCursor::maxTimeMS() times out during OP_QUERY
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

configureFailPoint($mc, 'maxTimeAlwaysTimeOut', 1);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();
$collection->insert(array('x' => 1));
$collection->insert(array('x' => 2));

printf("\nQuerying without maxTimeMS\n");

$cursor = $collection->find();
$results = iterator_to_array($cursor);
printf("Found %d documents\n", count($results));

printf("\nQuerying with maxTimeMS\n");

$cursor = $collection->find();
$cursor->maxtimeMS(1000);
try {
    $results = iterator_to_array($cursor);
    printf("Found %d documents\n", count($results));
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}
?>
===DONE===
--EXPECTF--
Configured maxTimeAlwaysTimeOut fail point

Querying without maxTimeMS
Found 2 documents

Querying with maxTimeMS
exception class: MongoExecutionTimeoutException
exception message: %s:%d: operation exceeded time limit
exception code: 50
===DONE===
