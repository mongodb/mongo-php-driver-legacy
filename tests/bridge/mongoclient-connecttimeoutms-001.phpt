--TEST--
Test for PHP-1350: Functional test for MongoClient connectTimeoutMS option
--SKIPIF--
<?php require_once "tests/utils/bridge.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getBridgeInfo();

try {
    $m = new MongoClient($dsn, array('connectTimeoutMS' => 500));
} catch (MongoConnectionException $e) {
    echo $e->getCode(), ': ', $e->getMessage(), "\n";
}

?>
===DONE===
--EXPECTF--
71: Failed to connect to: %s:%d: Read timed out after reading 0 bytes, waited for 0.500000 seconds
===DONE===
