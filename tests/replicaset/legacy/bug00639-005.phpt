--TEST--
Test for PHP-639: MongoCursor::slaveOkay() has no effect (inherited from client)
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
--INI--
mongo.long_as_object=1
--FILE--
<?php
$mentions = array(); 
require_once "tests/utils/server.inc";

// Has to be old_mongo here, as MongoClient doesn't have the deprecated setSlaveOkay() method.
$m = old_mongo();

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

$m->setSlaveOkay(true);
$db = $m->selectDB(dbname());
$col = $db->bug639;

$cursor = $col->find(array(), array('email' => true));
$cursor->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";

$m->setSlaveOkay(false);
$db = $m->selectDB(dbname());
$col = $db->bug639;

$cursor = $col->find(array(), array('email' => true));
$cursor->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";
?>
--EXPECTF--
%s: %s: The Mongo class is deprecated, please use the MongoClient class in %sserver.inc on line %d

Deprecated: Function Mongo::setSlaveOkay() is deprecated in %s
pick server: random element %d while ignoring the primary
- connection: type: SECONDARY, socket: %d, ping: %d, hash: %s
connection type: SECONDARY

Deprecated: Function Mongo::setSlaveOkay() is deprecated in %s
pick server: random element %d
- connection: type: PRIMARY, socket: %d, ping: %d, hash: %s
connection type: PRIMARY
