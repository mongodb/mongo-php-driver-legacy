--TEST--
MongoCollection::count() supports maxTimeMS option
--SKIPIF--
<?php $needs = "2.6.0"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

configureFailPoint($mc, 'maxTimeAlwaysTimeOut', 1);

$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->insert(array('x' => 1));

try {
    $count = $c->count(array(), array('maxTimeMS' => 1000));
    printf("Found %d documents\n", $count);
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECTF--
Configured maxTimeAlwaysTimeOut fail point
exception class: MongoExecutionTimeoutException
exception message: %s:%d: operation exceeded time limit
exception code: 50
===DONE===
