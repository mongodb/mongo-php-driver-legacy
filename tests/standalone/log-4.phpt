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
Mongo::__construct(): is_ping: last pinged at %d; time: %dms
Fine:
Mongo::__construct(): mongo_get_connection: finding a STANDALONE connection
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): get_server_flags: start
Mongo::__construct(): send_packet: read from header: 36
Mongo::__construct(): send_packet: data_size: 51
Info:
Mongo::__construct(): Parsing mongodb://%s:%d
Mongo::__construct(): - Found node: %s:%d
Mongo::__construct(): is_master: setting maxBsonObjectSize to 16777216
