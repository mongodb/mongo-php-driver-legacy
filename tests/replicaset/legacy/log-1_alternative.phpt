--TEST--
Test for MongoLog
--SKIPIF--
<?php require "tests/utils/replicaset.inc"; ?>
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
$config = MongoShellServer::getReplicasetInfo();
$m = new Mongo($config["hosts"][0] . "/?replicaSet=" . $config["rsname"]); 
?>
===DONE===
<?php exit(0) ?>
--EXPECTF--
PARSE   INFO: Parsing %s:%d/?replicaSet=REPLICASET
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'replicaSet': 'REPLICASET'
PARSE   INFO: - Switching connection type: REPLSET
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
<<<<<<< HEAD
CON     FINE: get_server_flags: setting maxMessageSizeBytes to 48000000
CON     FINE: get_server_flags: added tag dc:west
CON     FINE: get_server_flags: added tag use:accounting
=======
>>>>>>> Fix tests
CON     INFO: is_ping: pinging %s:%d;REPLICASET;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;X;%d
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: ismaster: set name: REPLICASET, ismaster: 1, secondary: 0, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
<<<<<<< HEAD
CON     FINE: get_server_flags: setting maxMessageSizeBytes to 48000000
CON     FINE: get_server_flags: added tag dc:east
CON     FINE: get_server_flags: added tag use:reporting
=======
CON     INFO: is_ping: pinging %s:%d;REPLICASET;X;%d
>>>>>>> Fix tests
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     INFO: is_ping: pinging %s:%d;REPLICASET;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     INFO: is_ping: pinging %s:%d;REPLICASET;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;X;%d
CON     FINE: found connection %s:%d;REPLICASET;X;%d (looking for %s:%d;REPLICASET;X;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: ismaster: set name: REPLICASET, ismaster: 0, secondary: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
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
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: ismaster: set name: REPLICASET, ismaster: 0, secondary: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
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
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: ismaster: set name: REPLICASET, ismaster: 0, secondary: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
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
REPLSET FINE: filter_connections: done
REPLSET FINE: limiting to servers with same replicaset name
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE: limiting to servers with same replicaset name: done
REPLSET FINE: limiting by credentials
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE: limiting by credentials: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is 0ms
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
REPLSET FINE: selecting near server: done
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: PRIMARY, socket: %d, ping: 0, hash: %s:%d;REPLICASET;X;%d
===DONE===

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;REPLICASET;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;REPLICASET;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;REPLICASET;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;REPLICASET;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;REPLICASET;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;REPLICASET;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;REPLICASET;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;REPLICASET;X;%d in Unknown on line 0
