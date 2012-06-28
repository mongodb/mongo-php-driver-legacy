--TEST--
MongoCollection::getServer()
--DESCRIPTION--
Test for error when trying to insert something in a collection when
no server is attached to it.
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
--EXPECT--
Warning: MongoCollection::__construct() expects exactly 2 parameters, 0 given in /home/testfest/mongo-php-driver/tests/mongocollection-getserver_error.php on line 4
The MongoCollection object has not been correctly initialized by its constructor
