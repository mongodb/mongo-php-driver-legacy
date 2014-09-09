--TEST--
Test for PHP-543: Mongo::connect() should return a bool value.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone(null, true, true, array( 'connect' => false ) );
var_dump($m->connect());

try
{
	$m = new MongoClient("mongodb://totallynonsense/", array( 'connect' => false ) );
	var_dump(@$m->connect());
}
catch ( Exception $e )
{
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
bool(true)
Failed to connect to: %s:%d: %s
