--TEST--
MongoClient::selectDB
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$db = $m->selectDB();
echo is_object($db) ? '1' : '0', "\n";

$db = $m->selectDB(array('test'));
echo is_object($db) ? '1' : '0', "\n";

try {
	$db = $m->selectDB(NULL);
} catch(Exception $e) {
	echo $e->getMessage() . ".\n";
}

$db = $m->selectDB(new stdClass);
echo is_object($db) ? '1' : '0', "\n";
?>
--EXPECTF--
Warning: MongoClient::selectDB() expects exactly 1 parameter, 0 given in %smongoclient-selectdb_error.php on line %d
0

Warning: MongoClient::selectDB() expects parameter 1 to be string, array given in %smongoclient-selectdb_error.php on line %d
0
MongoDB::__construct(): invalid name .

Warning: MongoClient::selectDB() expects parameter 1 to be string, object given in %smongoclient-selectdb_error.php on line %d
0

