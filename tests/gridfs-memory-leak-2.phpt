--TEST--
GridFS: Testing minor memory issue
--FILE--
<?php
$conn = new Mongo;
$db   = $conn->phpunit;

$grid = $db->getGridFS();

$grid->storeBytes('some thing', array('filename' => '1.txt'));

echo "No memory leak";
--EXPECT--
No memory leak
