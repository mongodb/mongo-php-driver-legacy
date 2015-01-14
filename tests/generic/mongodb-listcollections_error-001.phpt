--TEST--
MongoDB::listCollections() requires "filter" option to be a document (legacy mode)
--SKIPIF--
<?php $needs = "2.7.5"; $needsOp = "<"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDB(dbname());

try {
    $db->listCollections(array(
        'filter' => 0,
    ));
} catch (MongoException $e) {
    printf("error message: %s\n", $e->getMessage());
    printf("error code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECT--
error message: Expected filter to be array or object, integer given
error code: 26
===DONE===
