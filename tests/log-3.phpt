--TEST--
Test for MongoLog (IO only)
--FILE--
<?php
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::IO);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://localhost");
?>
--EXPECTF--
Mongo::__construct(): parsing servers
Mongo::__construct(): current: localhost
Mongo::__construct(): done parsing
