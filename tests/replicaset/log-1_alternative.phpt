--TEST--
Test for MongoLog
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

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
$m = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT/?replicaSet=$REPLICASET_NAME");
?>
--EXPECTF--
Mongo::__construct(): Parsing mongodb://%s:%d/?replicaSet=%s
Mongo::__construct(): - Found node: %s:%d
Mongo::__construct(): - Found option 'replicaSet': %s
Mongo::__construct(): - Connection type: REPLSET
Mongo::__construct(): connection_create: creating new connection for %s:%d
Mongo::__construct(): get_connection_single: pinging %s:%d;X;%d
Mongo::__construct(): is_ping: start
Mongo::__construct(): is_ping: data_size: 17
Mongo::__construct(): is_ping: last pinged at %d; time: %dms
Mongo::__construct(): discover_topology: checking is_master for %s:%d;X;%d
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): is_master: start
Mongo::__construct(): send_packet: read from header: 36
Mongo::__construct(): send_packet: data_size: 266
Mongo::__construct(): is_master: the found replicaset name matches the expected one (%s).
Mongo::__construct(): is_master: setting maxBsonObjectSize to 16777216
Mongo::__construct(): is_master: set name: %s, is_master: 1, is_arbiter: 0
Mongo::__construct(): found host: %s:%d
Mongo::__construct(): found host: %s:%d
Mongo::__construct(): discover_topology: is_master worked
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): discover_topology: found new host: %s:%d
Mongo::__construct(): connection_create: creating new connection for %s:%d
Mongo::__construct(): get_connection_single: pinging %s:%d;X;%d
Mongo::__construct(): is_ping: start
Mongo::__construct(): is_ping: data_size: 17
Mongo::__construct(): is_ping: last pinged at %d; time: %dms
Mongo::__construct(): discover_topology: checking is_master for %s:%d;X;%d
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): is_master: start
Mongo::__construct(): send_packet: read from header: 36
Mongo::__construct(): send_packet: data_size: 268
Mongo::__construct(): is_master: the found replicaset name matches the expected one (%s).
Mongo::__construct(): is_master: setting maxBsonObjectSize to 16777216
Mongo::__construct(): is_master: set name: %s, is_master: 0, is_arbiter: 0
Mongo::__construct(): found host: %s:%d
Mongo::__construct(): found host: %s:%d
Mongo::__construct(): discover_topology: is_master worked
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): found connection %s:%d;X;%d (looking for %s:%d;X;%d)
Mongo::__construct(): finding candidate servers
Mongo::__construct(): - connection: type: PRIMARY  , socket: 3, ping: %d, hash: %s:%d;X;%d
Mongo::__construct(): select server: sorting
Mongo::__construct(): - connection: type: PRIMARY  , socket: 3, ping: %d, hash: %s:%d;X;%d
Mongo::__construct(): select server: only nearest
Mongo::__construct(): select server: nearest is %dms
Mongo::__construct(): - connection: type: PRIMARY  , socket: 3, ping: %d, hash: %s:%d;X;%d
Mongo::__construct(): pick server: random element 0
