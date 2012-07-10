--TEST--
Test for MongoLog (level variations)
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

echo "Warnings:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::WARNING);
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");

echo "Fine:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::FINE);
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");

echo "Info:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::INFO);
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
?>
--EXPECTF--
Warnings:
Fine:
Mongo::__construct(): parsing servers
Mongo::__construct(): current: %s
Mongo::__construct(): done parsing
Mongo::__construct(): %s:%d: pool get (%s)
Mongo::__construct(): %s:%d: pool connect (%s)
main(): %s:%d: pool done (%s)
Info:
