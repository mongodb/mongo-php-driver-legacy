--TEST--
Test for MongoLog (IO only)
--FILE--
<?php
require __DIR__ . "/../utils.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::IO);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
?>
--EXPECTF--
Mongo::__construct(): parsing servers
Mongo::__construct(): current: %s
Mongo::__construct(): done parsing
