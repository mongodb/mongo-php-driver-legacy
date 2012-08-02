--TEST--
MongoLog (all levels, parse module)
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
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
?>
--EXPECTF--
Mongo::__construct(): parsing servers
Mongo::__construct(): current: %s
Mongo::__construct(): done parsing
