--TEST--
Testing minor memory issue
--FILE--
<?php
$conn = new Mongo;
$db   = $conn->admin;

$grid = $db->getGridFS();

$grid->storeBytes('some thing', array('filename' => '1.txt'));

$grid->storeBytes('some thing', array('filename' => '1.txt'), array('safe' => true));

echo "No memory leak";
--EXPECT--
No memory leak
