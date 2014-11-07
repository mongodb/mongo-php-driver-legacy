--TEST--
MongoClient::selectCollection() with invalid database name
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

echo "Testing missing database name parameter\n";

$mc->selectCollection();

echo "\nTesting database name with invalid types\n";

$mc->selectCollection(array(), 'collection');
$mc->selectCollection(new stdClass, 'collection');

echo "\nTesting empty database name\n";

try {
    $mc->selectCollection('', 'collection');
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

echo "\nTesting database name with null bytes\n";

try {
    $mc->selectCollection("foo\0bar", 'collection');
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

echo "\nTesting database name with invalid characters\n";

try {
    $mc->selectCollection("foo.bar", 'collection');
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
--EXPECTF--
Testing missing database name parameter

Warning: MongoClient::selectCollection() expects exactly 2 parameters, 0 given in %s on line %d

Testing database name with invalid types

Warning: MongoClient::selectCollection() expects parameter 1 to be string, array given in %s on line %d

Warning: MongoClient::selectCollection() expects parameter 1 to be string, object given in %s on line %d

Testing empty database name
exception class: MongoException
exception message: Database name cannot be empty
exception code: 2

Testing database name with null bytes
exception class: MongoException
exception message: Database name cannot contain null bytes: foo\0...
exception code: 2

Testing database name with invalid characters
exception class: MongoException
exception message: Database name contains invalid characters: foo.bar
exception code: 2
