--TEST--
PHP-1019: Empty Batch Insert should throw exception
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();


try {
    $retval = $collection->batchInsert(array());
    var_dump($retval);
    echo "FAILED\n!";
} catch(MongoException $e) {
    echo $e->getMessage(), "\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
No write ops were included in the batch
===DONE===

