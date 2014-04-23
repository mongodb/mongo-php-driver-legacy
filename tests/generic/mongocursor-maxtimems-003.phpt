--TEST--
MongoCursor::maxTimeMS() times out during OP_GETMORE
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function setFailPoint() {
    global $mc;

    configureFailPoint($mc, 'maxTimeAlwaysTimeOut', 1);
}

$ctx = stream_context_create(array(
    'mongodb' => array('log_getmore' => 'setFailPoint'),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

foreach (range(1,5) as $i) {
    $collection->insert(array('x' => $i));
}

$cursor = $collection->find();
$cursor->batchSize(2);
$cursor->maxTimeMS(1000);

try {
    foreach ($cursor as $document) {
        printf("Found document: %d\n", $document['x']);
    }
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECTF--
Found document: 1
Found document: 2
Configured maxTimeAlwaysTimeOut fail point
exception class: MongoExecutionTimeoutException
exception message: %s:%d: operation exceeded time limit
exception code: 50
===DONE===
