--TEST--
MongoLog (all levels, all modules)
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

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
?>
--EXPECTF--
Mongo::__construct(): parsing servers
Mongo::__construct(): current: %s
Mongo::__construct(): done parsing
Mongo::__construct(): %s:%d: pool get (%s)
Mongo::__construct(): %s:%d: pool connect (%s)
Unknown: %s:%d: pool done (%s)
