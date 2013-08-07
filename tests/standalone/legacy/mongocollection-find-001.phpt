--TEST--
MongoCollection::findOne() with multiple queries.
--DESCRIPTION--
Here we test whether the ping is only done once every 5 seconds.
--SKIPIF--
skip Manual test
--FILE--
<?php
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);

require_once "tests/utils/server.inc";

$mongo = mongo_standalone();
$mongo->safe = true;
$mongo->setReadPreference(Mongo::RP_SECONDARY);

$coll1 = $mongo->selectCollection(dbname(), 'query');
echo "DROPPING:\n";
$coll1->drop();
echo "---\n";
$coll1->insert(array('_id' => 123, 'x' => 'foo'), array('safe' => 1));
echo "---\n";
$coll1->insert(array('_id' => 124, 'x' => 'foo'), array('safe' => 1));
echo "---\n";
$coll1->insert(array('_id' => 125, 'x' => 'foo'), array('safe' => 1));
echo "---\n";
die();

for ($i = 0; $i < 10; $i++) {
	$r = $coll1->find();
	sleep(2);
	echo "--------\n";
}
?>
--EXPECTF--
PARSE   INFO: Parsing mongodb://%s:%d/phpunit
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found database name 'phpunit'
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     INFO: connection_create: creating new connection for %s:%d
CON     INFO: ismaster: start
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 70
CON     FINE: ismaster: setting maxBsonObjectSize to 16777216
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: send_packet: read from header: 36
CON     FINE: send_packet: data_size: 17
CON     WARN: is_ping: last pinged at %d; time: %dms
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
DROPPING:
CON     INFO: forcing primary for command
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
---
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
CON     INFO: forcing primary for getlasterror
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
---
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
CON     INFO: forcing primary for getlasterror
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
---
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
CON     INFO: forcing primary for getlasterror
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON     FINE: found connection %s:%d;X;%d (looking for %s:%d;X;%d)
CON     FINE: is_ping: pinging %s:%d;X;%d
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: %d
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
REPLSET FINE: selecting near server: done
REPLSET FINE: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;X;%d
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
---

Notice: CON     FINE: mongo_connection_destroy: Closing socket for %s:%d;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;X;%d in Unknown on line 0
