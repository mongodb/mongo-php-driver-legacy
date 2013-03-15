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
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: setting maxMessageSizeBytes to 48000000
CON     INFO: is_ping: pinging %s:%d;-;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     INFO: is_ping: last pinged at %d; time: %dms

Notice: CON     FINE: mongo_connection_destroy: Destroying connection object for %s:%d;-;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;-;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;-;X;%d in Unknown on line 0
