--TEST--
Test for basic stream wrapper support
--FILE--
<?php
$conn = new Mongo();
$db   = $conn->selectDb('admin');
$grid = $db->getGridFs('wrapper');

// delete any previous results
$grid->drop();

// dummy file
$bytes = sha1(time());
$grid->storeBytes($bytes, array("filename" => "demo.txt"), array('safe' => true));

// fetch it
$file = $grid->findOne(array('filename' => 'demo.txt'));

// get file descriptor
$fp = $file->getResource();
var_dump($fp);
var_dump($bytes === fread($fp, 1024));

--EXPECTF--
resource(%d) of type (stream)
bool(true)
