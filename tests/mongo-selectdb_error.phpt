--TEST--
Mongo::selectDB
--FILE--
<?php
$m = new Mongo();
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
Warning: Mongo::selectDB() expects exactly 1 parameter, 0 given in %s/mongo-selectdb_error.php on line 3
0

Warning: Mongo::selectDB() expects parameter 1 to be string, array given in %s/mongo-selectdb_error.php on line 6
0
MongoDB::__construct(): invalid name .

Warning: Mongo::selectDB() expects parameter 1 to be string, object given in %s/mongo-selectdb_error.php on line 15
0

