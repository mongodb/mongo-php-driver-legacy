--TEST--
Test for PHP-639: MongoCursor::slaveOkay() has no effect (static on, then method off)
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
--INI--
mongo.long_as_object=1
--FILE--
<?php
$mentions = array(); 
require_once "tests/utils/server.inc";

$m = mongo();
$db = $m->selectDB(dbname());
$col = $db->bug639;

MongoLog::setModule( MongoLog::ALL );
MongoLog::setLevel( MongoLog::ALL );

$showNext = false;

MongoLog::setCallback( function($a, $b, $message) use (&$showNext) {
	if ($showNext) {
		echo $message, "\n";
	}
	$showNext = false;
	if (preg_match('/^pick server/', $message)) {
		$showNext = true;
		echo $message, "\n";
	}
} );

MongoCursor::$slaveOkay = true;

$cursor = $col->find(array(), array('email' => true));
$cursor->slaveOkay(false)->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";
?>
--EXPECTF--
%s: Function MongoCursor::slaveOkay() is deprecated in %s on line %d
pick server: random element %d
- connection: type: PRIMARY, socket: %d, ping: %d, hash: %s
connection type: PRIMARY
