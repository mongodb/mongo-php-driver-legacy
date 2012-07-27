--TEST--
Test for MongoLog (connection only)
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

MongoLog::setModule(MongoLog::CON);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
?>
--EXPECTF--
Mongo::__construct(): connection_create: creating new connection for %s:%d
Mongo::__construct(): get_connection_single: pinging %s:%d;X;%d
Mongo::__construct(): is_ping: start
Mongo::__construct(): is_ping: data_size: 17
Mongo::__construct(): is_ping: last pinged at 1%d; time: 0ms
Mongo::__construct(): get_server_flags: start
Mongo::__construct(): get_server_flags: read from header: 36
Mongo::__construct(): get_server_flags: data_size: 51
Mongo::__construct(): get_server_flags: setting maxBsonObjectSize to %d

Notice: PHP Shutdown: freeing connection %s:%d;X;%d in Unknown on line 0
