--TEST--
MongoClient::selectCollection() with invalid collection name
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

echo "Testing missing collection name parameter\n";

$mc->selectCollection('db');

echo "\nTesting collection name with invalid types\n";

$mc->selectCollection('db', array());
$mc->selectCollection('db', new stdClass);

echo "\nTesting empty collection name\n";

try {
    $mc->selectCollection('db', '');
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

echo "\nTesting collection name with null bytes\n";

try {
    $mc->selectCollection('db', "foo\0bar");
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
--EXPECTF--
Testing missing collection name parameter

Warning: MongoClient::selectCollection() expects exactly 2 parameters, 1 given in %s on line %d

Testing collection name with invalid types

Warning: MongoClient::selectCollection() expects parameter 2 to be string, array given in %s on line %d

Warning: MongoClient::selectCollection() expects parameter 2 to be string, object given in %s on line %d

Testing empty collection name
exception class: MongoException
exception message: Collection name cannot be empty
exception code: 2

Testing collection name with null bytes
exception class: MongoException
exception message: Collection name cannot contain null bytes: foo\0...
exception code: 2
