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
Mongo::__construct(): is_ping: last pinged at %d; time: %dms
Mongo::__construct(): is_ping: last pinged at %d; time: %dms
Fine:
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): is_master: start
Mongo::__construct(): send_packet: read from header: 36
Mongo::__construct(): send_packet: data_size: 266
Mongo::__construct(): is_master: the found replicaset name matches the expected one (RS).
Mongo::__construct(): is_master: set name: RS, is_master: 1, is_arbiter: 0
Mongo::__construct(): discover_topology: is_master worked
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): finding candidate servers
Mongo::__construct(): select server: sorting
Mongo::__construct(): select server: only nearest
Info:
Mongo::__construct(): Parsing mongodb://%s:%d
Mongo::__construct(): - Found node: %s:%d
Mongo::__construct(): - Connection type: STANDALONE
Mongo::__construct(): discover_topology: checking is_master for %s:%d;X;%d
Mongo::__construct(): is_master: setting maxBsonObjectSize to 16777216
Mongo::__construct(): found host: %s:%d
Mongo::__construct(): found host: %s:%d
Mongo::__construct(): - connection: type: PRIMARY  , socket: 3, ping: %d, hash: %s:%d;X;%d
Mongo::__construct(): - connection: type: PRIMARY  , socket: 3, ping: %d, hash: %s:%d;X;%d
Mongo::__construct(): select server: nearest is %dms
Mongo::__construct(): - connection: type: PRIMARY  , socket: 3, ping: %d, hash: %s:%d;X;%d
Mongo::__construct(): pick server: random element 0
