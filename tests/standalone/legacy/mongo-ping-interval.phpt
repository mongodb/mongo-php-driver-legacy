--TEST--
mongo.ping_interval
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--INI--
mongo.ping_interval=103
--FILE--
<?php
function error_handler($code, $message)
{
	if ( preg_match( '/is_ping.*left: (\d+)/', $message, $m ) )
	{
		echo "LEFT: {$m[1]}\n";
	}
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::CON);
MongoLog::setLevel(MongoLog::FINE);

require_once "tests/utils/server.inc";

$mongo = mongo_standalone();

$coll1 = $mongo->selectCollection(dbname(), 'query');
$coll1->drop();
echo "---\n";
ini_set( 'mongo.ping_interval', 83 );
$coll1->insert(array('_id' => 125, 'x' => 'foo'), array('safe' => 1));

MongoLog::setModule(MongoLog::NONE);
MongoLog::setLevel(MongoLog::NONE);
?>
--EXPECTF--
LEFT: 103
---
LEFT: 83
