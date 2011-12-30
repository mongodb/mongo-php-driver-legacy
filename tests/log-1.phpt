--TEST--
Test for MongoLog
--FILE--
<?php
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://localhost");
?>
--EXPECTF--
Mongo::__construct(): parsing servers
Mongo::__construct(): current: localhost
Mongo::__construct(): done parsing
Mongo::__construct(): localhost:27017: pool get (%s)
Mongo::__construct(): localhost:27017: pool connect (%s)
Unknown: localhost:27017: pool done (%s)
