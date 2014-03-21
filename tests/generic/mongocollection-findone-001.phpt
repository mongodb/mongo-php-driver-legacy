--TEST--
MongoCollection::findOne() maxTimeMS option times out during OP_QUERY
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

$result = $mc->admin->command(array(
    'configureFailPoint' => 'maxTimeAlwaysTimeOut',
    'mode' => array('times' => 1),
));

if ( ! empty($result['ok'])) {
    printf("\nActivated maxTimeAlwaysTimeOut fail point\n");
} else {
    printf("\nError setting maxTimeAlwaysTimeOut fail point\n");
}

printf("\nQuerying with maxTimeMS (using fail point)\n");

try {
    $document = $collection->findOne(array(), array(), array('maxTimeMS' => 1000));
    printf("Found document with x=%d\n", $document['x']);
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

printf("\nQuerying without maxTimeMS\n");

$document = $collection->findOne();
printf("Found document with x=%d\n", $document['x']);
?>
===DONE===
--EXPECTF--
Activated maxTimeAlwaysTimeOut fail point

Querying with maxTimeMS (using fail point)
exception class: MongoExecutionTimeoutException
exception message: %s:%d: operation exceeded time limit
exception code: 50

Querying without maxTimeMS
Found document with x=1
===DONE===
