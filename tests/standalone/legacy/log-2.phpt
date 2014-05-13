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
CON     INFO: connection_create: creating new connection for %s:%d0
CON     INFO: stream_connect: Not establishing SSL for %s:%d0
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: %d
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: %sminWireVersion%s
CON     FINE: ismaster: %smaxWireVersion%s
CON     FINE: ismaster: %smaxBsonObjectSize%s
CON     FINE: ismaster: %smaxMessageSizeBytes%s
CON     FINE: ismaster: %smaxWriteBatchSize%s
CON     INFO: ismaster: set name: (null), ismaster: 1, secondary: 0, is_arbiter: 0
CON     INFO: ismaster: last ran at %d
CON     INFO: get_server_version: start
CON     FINE: send_packet: read from header: %d
CON     FINE: send_packet: data_size: %d
CON     INFO: get_server_version: server version: %s
CON     INFO: is_ping: pinging %s:%d0;-;.;%d
CON     FINE: send_packet: read from header: %d
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: %dms
CON     FINE: ismaster: skipping: last ran at %d, now: %d, time left: %d
