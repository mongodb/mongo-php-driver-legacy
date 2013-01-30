--TEST--
MongoCursor::setReadPreference [1]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$mentions = array(); 
require_once dirname(__FILE__) . "/../utils.inc";

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
$cursor->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED)->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";

$cursor = $col->find(array(), array('email' => true));
$cursor->setReadPreference(MongoClient::RP_PRIMARY)->limit(1);
iterator_to_array($cursor);
$info = $cursor->info();
echo "connection type: ", $info['connection_type_desc'], "\n";
?>
--EXPECTF--
pick server: random element %d while ignoring the primary
- connection: type: SECONDARY, socket: %d, ping: %d, hash: %s
connection type: SECONDARY
pick server: random element %d
- connection: type: PRIMARY, socket: %d, ping: %d, hash: %s
connection type: PRIMARY
