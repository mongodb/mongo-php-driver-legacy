--TEST--
Test for MongoLog (level variations)
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

echo "Warnings:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::WARNING);
$m = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT", array("replicaSet" => $REPLICASET_NAME));

echo "Fine:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::FINE);
$m = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT", array("replicaSet" => $REPLICASET_NAME));

echo "Info:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::INFO);
$m = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT", array("replicaSet" => $REPLICASET_NAME));
?>
--EXPECTF--
Warnings:
Fine:
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     FINE: discover_topology: checking ismaster for %s:%d;%s;X;%d
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 278
CON     FINE: ismaster: the server name matches what we thought it'd be (%s:%d).
CON     FINE: ismaster: the found replicaset name matches the expected one (%s).
CON     FINE: discover_topology: ismaster worked
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
CON     FINE: found connection %s:%d;%s;X;%d (looking for %s:%d;%s;X;%d)
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: PRIMARY, socket: 3, ping: 0, hash: %s:%d;%s;X;%d
REPLSET FINE:   - tag: dc:west
REPLSET FINE:   - tag: use:accounting
REPLSET FINE: filter_connections: done
REPLSET FINE: limiting to servers with same replicaset name
REPLSET FINE: - connection: type: PRIMARY, socket: 3, ping: 0, hash: %s:%d;%s;X;%d
REPLSET FINE:   - tag: dc:west
REPLSET FINE:   - tag: use:accounting
REPLSET FINE: limiting to servers with same replicaset name: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: PRIMARY, socket: 3, ping: 0, hash: %s:%d;%s;X;%d
REPLSET FINE:   - tag: dc:west
REPLSET FINE:   - tag: use:accounting
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is 0ms
REPLSET FINE: - connection: type: PRIMARY, socket: 3, ping: 0, hash: %s:%d;%s;X;%d
REPLSET FINE:   - tag: dc:west
REPLSET FINE:   - tag: use:accounting
REPLSET FINE: selecting near server: done
Info:
PARSE   INFO: Parsing mongodb://%s:%d
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'replicaSet': '%s'
PARSE   INFO: - Switching connection type: REPLSET
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
CON     INFO: is_ping: pinging %s:%d;%s;X;%d
CON     INFO: is_ping: last pinged at %d; time: 0ms
CON     INFO: ismaster: start
CON     INFO: ismaster: set name: %s, ismaster: 1, is_arbiter: 0
CON     INFO: found host: %s:%d
CON     INFO: found host: %s:%d
CON     INFO: ismaster: last ran at %d
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: PRIMARY, socket: 3, ping: 0, hash: %s:%d;%s;X;%d
REPLSET INFO:   - tag: dc:west
REPLSET INFO:   - tag: use:accounting

Notice: CON     INFO: freeing connection %s:%d;%s;X;%d in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;%s;X;%d in Unknown on line 0
