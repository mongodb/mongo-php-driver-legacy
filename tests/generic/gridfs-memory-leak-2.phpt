--TEST--
GridFS: Testing minor memory issue
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$conn = Mongo();
$db   = $conn->phpunit;

$grid = $db->getGridFS();

$grid->storeBytes('some thing', array('filename' => '1.txt'));

echo "No memory leak";
--EXPECT--
No memory leak
