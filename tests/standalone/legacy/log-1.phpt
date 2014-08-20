--TEST--
Test for MongoLog
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient("mongodb://$dsn", array('connectTimeoutMS' => 30000));
MongoLog::setModule(0);
MongoLog::setLevel(0);
?>
--EXPECTF--
PARSE   INFO: Parsing mongodb://127.0.0.1:30000
PARSE   INFO: - Found node: 127.0.0.1:30000
PARSE   INFO: - Connection type: %s
PARSE   INFO: - Found option 'connectTimeoutMS': 30000
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     INFO: connection_create: creating new connection for 127.0.0.1:30000
CON     FINE: Connecting to tcp://127.0.0.1:30000 (127.0.0.1:30000;-;.;%d) with connection timeout: 30.000000
CON     INFO: stream_connect: Not establishing SSL for 127.0.0.1:30000
CON     FINE: Setting stream timeout to 30.000000
CON     INFO: ismaster: start
CON     FINE: No timeout changes for 127.0.0.1:30000;-;.;%d
CON     FINE: send_packet: read from header: %d
CON     FINE: send_packet: data_size: %d
CON     FINE: No timeout changes for 127.0.0.1:30000;-;.;%d
CON     FINE: ismaster: %sminWireVersion%s
CON     FINE: ismaster: %smaxWireVersion%s
CON     FINE: ismaster: %smaxBsonObjectSize%s
CON     FINE: ismaster: %smaxMessageSizeBytes%s
CON     FINE: ismaster: %smaxWriteBatchSize%s
CON     INFO: ismaster: set name: (null), ismaster: 1, secondary: 0, is_arbiter: 0
CON     INFO: ismaster: last ran at %d
CON     INFO: get_server_version: start
CON     FINE: No timeout changes for 127.0.0.1:30000;-;.;%d
CON     FINE: send_packet: read from header: %d
CON     FINE: send_packet: data_size: %d
CON     FINE: No timeout changes for 127.0.0.1:30000;-;.;%d
CON     INFO: get_server_version: server version: %s
CON     INFO: is_ping: pinging 127.0.0.1:30000;-;.;%d
CON     FINE: No timeout changes for 127.0.0.1:30000;-;.;%d
CON     FINE: send_packet: read from header: %d
CON     FINE: send_packet: data_size: %d
CON     FINE: No timeout changes for 127.0.0.1:30000;-;.;%d
CON     INFO: is_ping: last pinged at %d; time: %dms
CON     FINE: ismaster: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: - collect any
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: 127.0.0.1:30000;-;.;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: limiting by seeded/discovered servers
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: 127.0.0.1:30000;-;.;%d
REPLSET FINE: limiting by seeded/discovered servers: done
REPLSET FINE: limiting by credentials
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: 127.0.0.1:30000;-;.;%d
REPLSET FINE: limiting by credentials: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: 127.0.0.1:30000;-;.;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: 127.0.0.1:30000;-;.;%d
REPLSET FINE: selecting near server: done
REPLSET INFO: pick server: random element %d
REPLSET INFO: - connection: type: %s, socket: 42, ping: %d, hash: 127.0.0.1:30000;-;.;%d
