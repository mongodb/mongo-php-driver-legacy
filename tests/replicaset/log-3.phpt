--TEST--
Test for MongoLog (PARSE only)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::PARSE);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT", array("replicaSet" => $REPLICASET_NAME));
?>
--EXPECTF--
PARSE	INFO: Parsing mongodb://%s:%d
PARSE	INFO: - Found node: %s:%d
PARSE	INFO: - Connection type: STANDALONE
PARSE	INFO: - Found option 'replicaSet': '%s'
PARSE	INFO: - Switching connection type: REPLSET
