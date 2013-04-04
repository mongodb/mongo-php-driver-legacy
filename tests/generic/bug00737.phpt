--TEST--
Test for PHP-737: Missing RP tags parse error message using MongoClient options array
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();

try {
    new MongoClient($dsn, array('readPreferenceTags' => ','));
} catch (MongoConnectionException $e) {
    printf("error message: %s\n", $e->getMessage());
    printf("error code: %d\n", $e->getCode());
}

try {
    new MongoClient('mongodb://' . $dsn . '/?readPreferenceTags=,');
} catch (MongoConnectionException $e) {
    printf("error message: %s\n", $e->getMessage());
    printf("error code: %d\n", $e->getCode());
}

?>
--EXPECTF--
error message: Error while trying to parse tags: No separator for '%s'
error code: 23
error message: Error while trying to parse tags: No separator for '%s'
error code: 23
