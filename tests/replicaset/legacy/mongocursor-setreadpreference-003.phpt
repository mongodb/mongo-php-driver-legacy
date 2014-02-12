--TEST--
MongoCursor::setReadPreference (first slaveOkay, then setReadPreference) (3)
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php require_once "tests/utils/replicaset.inc"; ?>
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

$cursor = $col->find(array(), array('email' => true));
$cursor->slaveOkay(false)->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED)->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";

$cursor = $col->find(array(), array('email' => true));
$cursor->slaveOkay(false)->setReadPreference(MongoClient::RP_PRIMARY)->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";

$cursor = $col->find(array(), array('email' => true));
$cursor->slaveOkay(true)->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED)->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";

$cursor = $col->find(array(), array('email' => true));
$cursor->slaveOkay(true)->setReadPreference(MongoClient::RP_PRIMARY)->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";
?>
--EXPECTF--
%s: Function MongoCursor::slaveOkay() is deprecated in %s on line %d
pick server: random element %d while ignoring the primary
- connection: type: SECONDARY, socket: %d, ping: %d, hash: %s
connection type: SECONDARY

%s: Function MongoCursor::slaveOkay() is deprecated in %s on line %d
pick server: random element %d
- connection: type: PRIMARY, socket: %d, ping: %d, hash: %s
connection type: PRIMARY

%s: Function MongoCursor::slaveOkay() is deprecated in %s on line %d
pick server: random element %d while ignoring the primary
- connection: type: SECONDARY, socket: %d, ping: %d, hash: %s
connection type: SECONDARY

%s: Function MongoCursor::slaveOkay() is deprecated in %s on line %d
pick server: random element %d
- connection: type: PRIMARY, socket: %d, ping: %d, hash: %s
connection type: PRIMARY
