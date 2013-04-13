--TEST--
Test for MongoLog
--SKIPIF--
<?php require "tests/utils/replicaset.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);

$config = MongoShellServer::getReplicasetInfo();
$m = new Mongo("mongodb://" . $config["hosts"][0], array("replicaSet" => $config["rsname"]));

MongoLog::setModule(0);
MongoLog::setLevel(0);
?>
--EXPECTF--
PARSE   INFO: Parsing mongodb://%s:%d
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'replicaSet': '%s'
PARSE   INFO: - Switching connection type: REPLSET
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes %s
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;%s;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     FINE: discover_topology: checking ismaster for %s:%d;%s;X;%d
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (%s).
CON     INFO: ismaster: set name: %s, ismaster: 1, secondary: 0, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %s
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes %s
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;%s;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %s
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes %s
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;REPLICASET;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %s
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes %s
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;REPLICASET;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;X;%d
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %s
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: ismaster: set name: REPLICASET, ismaster: 0, secondary: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;X;%d
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %s
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: ismaster: set name: REPLICASET, ismaster: 0, secondary: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;X;%d
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %s
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: ismaster: set name: REPLICASET, ismaster: 0, secondary: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: filter_connections: done
REPLSET FINE: limiting to servers with same replicaset name
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: limiting to servers with same replicaset name: done
REPLSET FINE: limiting by credentials
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: limiting by credentials: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is 0ms
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: selecting near server: done
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET INFO:   - tag: dc:%s
REPLSET INFO:   - tag: server:%d
