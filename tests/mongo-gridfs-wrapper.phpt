--TEST--
Test for basic stream wrapper support
--FILE--
<?php
$conn = new Mongo();
$db   = $conn->selectDb('admin');
$grid = $db->getGridFs('wrapper');

$bytes = sha1(time());
$grid->storeBytes($bytes, array("filename" => "demo.txt"));

$file = $grid->findOne(array('filename' => 'demo.txt'));
$fp = $file->getResource();
var_dump($fp);
var_dump($file);

#fread($fp, 1024);

--EXPECTF--
resource(%d) of type (stream)
