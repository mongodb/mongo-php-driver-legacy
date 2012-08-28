--TEST--
Test for MongoLog
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT/?replicaSet=$REPLICASET_NAME");
?>
===DONE===
<?php exit(0) ?>
--EXPECTF--
PARSE   INFO: Parsing mongodb://%s:%d/?replicaSet=%s
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'replicaSet': '%s'
PARSE   INFO: - Switching connection type: REPLSET
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 266
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     INFO: get_connection_single: pinging %s:%d;X;%d
CON     FINE: is_ping: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     WARN: is_ping: last pinged at %d; time: %dms
CON     FINE: discover_topology: checking is_master for %s:%d;X;%d
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     INFO: rs_status: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 626
CON     FINE: rs_status: the found replicaset name matches the expected one (%s).
CON     FINE: rs_status: the server name matches what we thought it'd be (%s:%d).
CON     INFO: rs_status: found a connectable host: %s:%d (state: 1)
CON     INFO: rs_status: found a connectable host: %s:%d (state: 2)
CON     WARN: rs_status: found an unconnectable host: %s:%d (state: 8)
CON     INFO: rs_status: last ran at %d
CON     INFO: discover_topology: is_master worked
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     INFO: discover_topology: found new host: %s:%d
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: get_server_flags: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 268
CON     FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON     INFO: get_connection_single: pinging %s:%d;X;%d
CON     FINE: is_ping: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     WARN: is_ping: last pinged at %d; time: %dms
CON     FINE: discover_topology: checking is_master for %s:%d;X;%d
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     INFO: rs_status: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 664
CON     FINE: rs_status: the found replicaset name matches the expected one (%s).
CON     INFO: rs_status: found a connectable host: %s:%d (state: 1)
CON     FINE: rs_status: the server name matches what we thought it'd be (%s:%d).
CON     INFO: rs_status: found a connectable host: %s:%d (state: 2)
CON     WARN: rs_status: found an unconnectable host: %s:%d (state: 8)
CON     INFO: rs_status: last ran at %d
CON     INFO: discover_topology: is_master worked
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: mongo_sort_servers: sorting
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;X;%d
REPLSET FINE: mongo_sort_servers: done
REPLSET FINE: select server: only nearest
REPLSET FINE: select server: nearest is %dms
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;X;%d
===DONE===

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;X;%d in Unknown on line 0

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;X;%d in Unknown on line 0
