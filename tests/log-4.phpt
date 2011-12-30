--TEST--
Test for MongoLog (level variations)
--FILE--
<?php
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

echo "Warnings:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::WARNING);
$m = new Mongo("mongodb://localhost");

echo "Fine:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::FINE);
$m = new Mongo("mongodb://localhost");

echo "Info:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::INFO);
$m = new Mongo("mongodb://localhost");
?>
--EXPECTF--
Warnings:
Fine:
Mongo::__construct(): parsing servers
Mongo::__construct(): current: localhost
Mongo::__construct(): done parsing
Mongo::__construct(): localhost:27017: pool get (%s)
Mongo::__construct(): localhost:27017: pool connect (%s)
main(): localhost:27017: pool done (%s)
Info:
