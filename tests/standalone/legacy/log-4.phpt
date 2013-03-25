--TEST--
Test for MongoLog (level variations)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

echo "Warnings:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::WARNING);
$dsn = MongoShellServer::getStandaloneInfo();
$m = new Mongo("mongodb://$dsn");

echo "Fine:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::FINE);
$dsn = MongoShellServer::getStandaloneInfo();
$m = new Mongo("mongodb://$dsn");

echo "Info:\n";
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::INFO);
$dsn = MongoShellServer::getStandaloneInfo();
$m = new Mongo("mongodb://$dsn");
?>
--EXPECTF--
Warnings:
Fine:
CON     FINE: found connection %s:%d;-;X;%d (looking for %s:%d;-;X;%d)
CON     FINE: is_ping: skipping: last ran at %d, now: %d, time left: 5
REPLSET FINE: finding candidate servers
REPLSET FINE: - all servers
REPLSET FINE: filter_connections: adding connections:
REPLSET FINE: - connection: type: STANDALONE, socket: %d, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: filter_connections: done
REPLSET FINE: limiting by seeded/discovered servers
REPLSET FINE: - connection: type: STANDALONE, socket: %d, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: limiting by seeded/discovered servers: done
REPLSET FINE: limiting by credentials
REPLSET FINE: - connection: type: STANDALONE, socket: %d, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: limiting by credentials: done
REPLSET FINE: sorting servers by priority and ping time
REPLSET FINE: - connection: type: STANDALONE, socket: %d, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: sorting servers: done
REPLSET FINE: selecting near servers
REPLSET FINE: selecting near servers: nearest is %dms
REPLSET FINE: - connection: type: STANDALONE, socket: %d, ping: %d, hash: %s:%d;-;X;%d
REPLSET FINE: selecting near server: done
Info:
PARSE   INFO: Parsing mongodb://%s:%d
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
CON     INFO: mongo_get_read_write_connection: finding a STANDALONE connection
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: STANDALONE, socket: %d, ping: %d, hash: %s:%d;-;X;%d

Notice: CON     INFO: freeing connection %s:%d;-;X;%d in Unknown on line 0
