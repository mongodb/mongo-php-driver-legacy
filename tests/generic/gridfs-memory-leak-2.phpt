--TEST--
GridFS: Testing minor memory issue
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$conn = Mongo();
$db	= $conn->phpunit;

$grid = $db->getGridFS();

$grid->storeBytes('some thing', array('filename' => '1.txt'));

echo "No memory leak";
--EXPECT--
No memory leak
