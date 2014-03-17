--TEST--
Test for MongoLog (level variations)
--SKIPIF--
<?php require "tests/utils/replicaset.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
$config = MongoShellServer::getReplicasetInfo();
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

echo "Warnings:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::WARNING);
$m = new MongoClient($config["hosts"][0], array("replicaSet" => $config["rsname"]));

echo "Fine:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::FINE);
$m = new MongoClient($config["hosts"][0], array("replicaSet" => $config["rsname"]));

echo "Info:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::INFO);
$m = new MongoClient("mongodb://" . $config["hosts"][0], array("replicaSet" => $config["rsname"]));
MongoLog::setModule(0);
MongoLog::setLevel(0);
?>
--EXPECTF--
Warnings:
Fine:
CON     FINE: found connection %s:%d;%s;.;%d (looking for %s:%d;%s;.;%d)
CON     FINE: found connection %s:%d;%s;.;%d (looking for %s:%d;%s;.;%d)
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
CON     FINE: discover_topology: checking ismaster for %s:%d;%s;.;%d
CON     FINE: found connection %s:%d;%s;.;%d (looking for %s:%d;%s;.;%d)
CON     FINE: ismaster: skipping: last ran at %d, now: %d, time left: %d
CON     FINE: discover_topology: ismaster got skipped
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: filter_connections: done
REPLSET FINE: limiting to servers with same replicaset name
REPLSET FINE: - connection: type: PRIMARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: limiting to servers with same replicaset name: done
REPLSET FINE: limiting by credentials
REPLSET FINE: - connection: type: PRIMARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: SECONDARY, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: limiting by credentials: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET FINE:   - tag: dc:%s
REPLSET FINE:   - tag: server:%d
REPLSET FINE: selecting near server: done
Info:
PARSE   INFO: Parsing mongodb://%s:%d
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'replicaSet': 'REPLICASET'
PARSE   INFO: - Switching connection type: REPLSET
CON     INFO: mongo_get_read_write_connection: finding a REPLSET connection (read)
REPLSET INFO: pick server: random element %d
REPLSET INFO: - connection: type: %s, socket: 42, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET INFO:   - tag: dc:%s
REPLSET INFO:   - tag: server:%d
