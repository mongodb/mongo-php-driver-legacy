--TEST--
MongoCollection::getServer()
--DESCRIPTION--
Test for error when trying to insert something in a collection when
no server is attached to it.
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
try
{
	$c = new MongoCollection();
	$c->insert(array('hello' => 'Hello, world!', 0 => 'Joehoe'));
}
catch (Exception $e)
{
	echo $e->getMessage();
}
?>
--EXPECTF--
Warning: MongoCollection::__construct() expects exactly 2 parameters, 0 given in %smongocollection-getserver_error.php on line %d
The MongoCollection object has not been correctly initialized by its constructor
