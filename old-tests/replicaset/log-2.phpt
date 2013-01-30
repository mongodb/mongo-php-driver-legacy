--TEST--
Test for MongoLog (connection only)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::CON);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT", array("replicaSet" => $REPLICASET_NAME));
?>
--EXPECTF--
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: setting maxMessageSizeBytes to 48000000
CON     FINE: get_server_flags: added tag dc:west
CON     FINE: get_server_flags: added tag use:accounting
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
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 277
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: setting maxMessageSizeBytes to 48000000
CON     FINE: get_server_flags: added tag dc:east
CON     FINE: get_server_flags: added tag use:reporting
CON     INFO: is_ping: pinging %s:%d;%s;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     FINE: discover_topology: checking ismaster for %s:%d;%s;X;%d
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 277
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (%s).
CON     INFO: ismaster: set name: %s, ismaster: 0, secondary: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)

Notice: CON     FINE: mongo_connection_destroy: Destroying connection object for %s:%d;%s;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;%s;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;%s;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Destroying connection object for %s:%d;%s;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;%s;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;%s;X;%d in Unknown on line 0
