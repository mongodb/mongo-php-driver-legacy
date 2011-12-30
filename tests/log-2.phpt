--TEST--
Test for MongoLog (pool only)
--FILE--
<?php
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::POOL);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://localhost");
?>
--EXPECTF--
Mongo::__construct(): localhost:27017: pool get (%s)
Mongo::__construct(): localhost:27017: pool connect (%s)
