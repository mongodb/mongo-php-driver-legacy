--TEST--
Test for MongoLog
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
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
$m = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
?>
--EXPECTF--
PARSE	 INFO: Parsing mongodb://%s:%d
PARSE	 INFO: - Found node: %s:%d
PARSE	 INFO: - Connection type: STANDALONE
CON		 INFO: mongo_get_read_write_connection: finding a STANDALONE connection
CON		 INFO: connection_create: creating new connection for %s:%d
CON		 INFO: get_server_flags: start
CON		 FINE: send_packet: read from header: 36
CON		 FINE: send_packet: data_size: %d
CON		 FINE: get_server_flags: setting maxBsonObjectSize to 16777216
CON		 INFO: is_ping: pinging %s:%d;-;X;%d
CON		 FINE: send_packet: read from header: 36
CON		 FINE: send_packet: data_size: 17
CON		 INFO: is_ping: last pinged at %d; time: %dms
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: limiting by seeded/discovered servers
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: limiting by seeded/discovered servers: done
REPLSET FINE: limiting by credentials
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: 0, hash: %s:%d;-;X;%d
REPLSET FINE: limiting by credentials: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: 3, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: selecting near server: done
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: 3, ping: %d, hash: %s:%d;-;X;%d

Notice: CON		 FINE: mongo_connection_destroy: Closing socket for %s:%d;-;X;%d. in Unknown on line 0

Notice: CON     INFO: freeing connection %s:%d;-;X;%d in Unknown on line 0
