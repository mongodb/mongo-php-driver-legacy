--TEST--
Test for MongoLog (connection only)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require "tests/utils/replicaset.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::CON);
MongoLog::setLevel(MongoLog::ALL);
$config = MongoShellServer::getReplicasetInfo();
$m = new Mongo($config["hosts"][0], array("replicaSet" => $config["rsname"]));
MongoLog::setModule(0);
MongoLog::setLevel(0);
?>
--EXPECTF--
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes%s
CON     FINE: get_server_flags: got replicaset name: REPLICASET
CON     INFO: get_server_flags: found server type: PRIMARY
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;REPLICASET;.;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: %dms
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;.;%d
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes%s
CON     INFO: get_server_flags: found server type: SECONDARY
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;REPLICASET;.;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: %dms
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes%s
CON     INFO: get_server_flags: found server type: SECONDARY
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;REPLICASET;.;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: %dms
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: stream_connect: Not establishing SSL for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     FINE: get_server_flags: %s maxMessageSizeBytes%s
CON     INFO: get_server_flags: found server type: SECONDARY
CON     FINE: get_server_flags: added tag dc:%s
CON     FINE: get_server_flags: added tag server:%d
CON     INFO: is_ping: pinging %s:%d;REPLICASET;.;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     INFO: is_ping: last pinged at %d; time: %dms
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;.;%d
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;.;%d
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: discover_topology: checking ismaster for %s:%d;REPLICASET;.;%d
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: %d
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (REPLICASET).
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d (passive)
CON     INFO: ismaster: last ran at %d
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)
CON     FINE: found connection %s:%d;REPLICASET;.;%d (looking for %s:%d;REPLICASET;.;%d)

