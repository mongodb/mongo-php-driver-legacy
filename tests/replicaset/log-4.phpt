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
CON     WARN: is_ping: last pinged at %d; time: %dms
CON     WARN: rs_status: found an unconnectable host: tertiary.rs.local:%d (state: 8)
CON     WARN: is_ping: last pinged at %d; time: %dms
CON     WARN: rs_status: found an unconnectable host: tertiary.rs.local:%d (state: 8)
Fine:
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: discover_topology: checking ismaster for %s:%d;X;%d
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: rs_status: skipping: last ran at %d, now: %d, time left: %d
CON     FINE: discover_topology: ismaster got skipped
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: PRIMARY, socket: 3, ping: %d, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: mongo_sort_servers: sorting
REPLSET FINE: - connection: type: PRIMARY, socket: 3, ping: %d, hash: %s:%d;X;%d
REPLSET FINE: mongo_sort_servers: done
REPLSET FINE: select server: only nearest
REPLSET FINE: select server: nearest is %dms
REPLSET FINE: pick server: random element 0
Info:
PARSE   INFO: Parsing mongodb://%s:%d
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'replicaSet': 'RS'
PARSE   INFO: - Switching connection type: REPLSET
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
REPLSET INFO: - connection: type: PRIMARY, socket: 3, ping: %d, hash: %s:%d;X;%d

Notice: CON     INFO: freeing connection secondary.rs.local:%d;X;%d in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;X;%d in Unknown on line 0
