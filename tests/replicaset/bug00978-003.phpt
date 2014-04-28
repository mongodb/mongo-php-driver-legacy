--TEST--
Test for PHP-978: Standalone connection to arbiter fails
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

MongoLog::setModule( MongoLog::RS );
MongoLog::setLevel( MongoLog::INFO | MongoLog::FINE );
set_error_handler('foo'); function foo($a, $b) {
	if (preg_match( '/^REPLSET FINE/', $b) && !preg_match('/(limiting|sorting|selecting|(connection: type))/', $b)) {
		return;
	}
	if (preg_match( '/^REPLSET INFO/', $b) && preg_match('/(tag)/', $b)) {
		return;
	}
	echo '  ', $b, "\n";
};

$arb = MongoShellServer::getAnArbiterNode();

echo "CREATING CONNECTION\n";
$m = new MongoClient("mongodb://$arb", array( 'replicaSet' => 'REPLICASET' ));
$c =  $m->selectDb(dbname())->bug978;

echo "INSERT DATA\n";
try {
	$m->selectDb(dbname())->bug978->insert(array("test" => 1));
} catch (MongoCursorException $e) {
	echo "\n", $e->getCode(), "\n";
	echo $e->getMessage(), "\n\n";
}

echo "RUNNING FIND\n";
$result = $c->findOne( array( 'foo' => 42 ) );

echo "RUNNING FIND WITH RP SECONDARY\n";
$c->setReadPreference( MongoClient::RP_SECONDARY );
$result = $c->findOne( array( 'foo' => 42 ) );
?>
--EXPECTF--
CREATING CONNECTION
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name: done
  REPLSET FINE: limiting by credentials
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting by credentials: done
  REPLSET FINE: sorting servers by priority and ping time
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: sorting servers: done
  REPLSET FINE: selecting near servers
  REPLSET FINE: selecting near servers: nearest is %dms
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: selecting near server: done
  REPLSET INFO: pick server: random element %d
  REPLSET INFO: - connection: type: %s, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
INSERT DATA
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name: done
  REPLSET FINE: limiting by credentials
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting by credentials: done
  REPLSET FINE: sorting servers by priority and ping time
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: sorting servers: done
  REPLSET FINE: selecting near servers
  REPLSET FINE: selecting near servers: nearest is %dms
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: selecting near server: done
  REPLSET INFO: pick server: random element 0
  REPLSET INFO: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
RUNNING FIND
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name: done
  REPLSET FINE: limiting by credentials
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting by credentials: done
  REPLSET FINE: sorting servers by priority and ping time
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: sorting servers: done
  REPLSET FINE: selecting near servers
  REPLSET FINE: selecting near servers: nearest is %dms
  REPLSET FINE: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: selecting near server: done
  REPLSET INFO: pick server: random element 0
  REPLSET INFO: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
RUNNING FIND WITH RP SECONDARY
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting to servers with same replicaset name: done
  REPLSET FINE: limiting by credentials
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: limiting by credentials: done
  REPLSET FINE: sorting servers by priority and ping time
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: sorting servers: done
  REPLSET FINE: selecting near servers
  REPLSET FINE: selecting near servers: nearest is %dms
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  REPLSET FINE: selecting near server: done
  REPLSET INFO: pick server: random element %d
  REPLSET INFO: - connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
