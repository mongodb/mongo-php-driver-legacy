--TEST--
mongo.is_master_interval
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
--INI--
mongo.is_master_interval=93
--FILE--
<?php
function error_handler($code, $message)
{
	if ( preg_match( '/ismaster:.*left: (\d+)/', $message, $m ) )
	{
		echo "LEFT: {$m[1]}\n";
	}
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::CON);
MongoLog::setLevel(MongoLog::FINE);

require_once "tests/utils/server.inc";

$mongo = mongo();

$coll1 = $mongo->selectCollection(dbname(), 'query');
$coll1->drop();
echo "---\n";
ini_set( 'mongo.is_master_interval', 73 );
$coll1->insert(array('_id' => 125, 'x' => 'foo'), array('safe' => 1));

MongoLog::setModule(MongoLog::NONE);
MongoLog::setLevel(MongoLog::NONE);
?>
--EXPECTF--
LEFT: 93
LEFT: 93
---
LEFT: 73
LEFT: 73
