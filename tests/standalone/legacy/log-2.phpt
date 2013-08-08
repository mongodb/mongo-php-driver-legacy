--TEST--
Test for MongoLog (connection only)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::CON);
MongoLog::setLevel(MongoLog::ALL);


$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

MongoLog::setModule(0);
MongoLog::setLevel(0);
?>
--EXPECTF--
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_version: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: get_server_version: server version: %d.%d.%d (%i)
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes%s
CON     INFO: get_server_flags: found server type: STANDALONE
CON     INFO: is_ping: pinging %s:%d;-;.;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     INFO: is_ping: last pinged at %d; time: %dms

