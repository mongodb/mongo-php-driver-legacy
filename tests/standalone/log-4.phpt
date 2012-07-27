--TEST--
Test for MongoLog (level variations)
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
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
Mongo::__construct(): is_ping: last pinged at 1%d; time: %dms
Fine:
Mongo::__construct(): found connection %s:27017;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): get_server_flags: start
Mongo::__construct(): get_server_flags: read from header: 36
Mongo::__construct(): get_server_flags: data_size: 51
Info:
Mongo::__construct(): Parsing mongodb://%s:%d
Mongo::__construct(): - Found node: %s:%d
Mongo::__construct(): - Connection type: STANDALONE
Mongo::__construct(): get_server_flags: setting maxBsonObjectSize to 16777216

Notice: PHP Shutdown: freeing connection %s:%d;X;%d in Unknown on line 0
