--TEST--
MongoDB::listCollections() requires "filter" option's "name" criteria to be a string (legacy mode)
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
        'filter' => array(
            'name' => new MongoRegex('/profile$/'),
        ),
    ));
} catch (MongoException $e) {
    printf("error message: %s\n", $e->getMessage());
    printf("error code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECT--
error message: Filter "name" must be a string for MongoDB <2.8, object given
error code: 27
===DONE===
