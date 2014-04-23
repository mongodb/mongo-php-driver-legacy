--TEST--
MongoCursor::killCursor() Testing faulty arguments
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$server = "This is a non existing hash";
$id     = 5;
var_dump(MongoClient::killCursor( $server, $id ));
?>
--EXPECTF--
Warning: MongoClient::killCursor(): A connection with hash 'This is a non existing hash' does not exist in %s on line %d
bool(false)
