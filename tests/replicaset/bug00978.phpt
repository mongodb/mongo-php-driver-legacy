--TEST--
Test for PHP-978: Standalone connection to arbiter fails
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

MongoLog::setModule( MongoLog::RS );
MongoLog::setLevel( MongoLog::INFO );
set_error_handler('foo'); function foo($a, $b) { echo $b, "\n"; };

$arb = MongoShellServer::getAnArbiterNode();
$m = new MongoClient("mongodb://$arb");
$result = $m->admin->command(array('replSetGetStatus' => 1));
if (!array_key_exists('ok', $result) || $result['ok'] == 0) {
	echo "error\n";
	var_dump($result);
} else {
	echo "all is good\n";
}
?>
--EXPECTF--
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: ARBITER, socket: %d, ping: 0, hash: %s:%d;-;.;%d
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: ARBITER, socket: %d, ping: 0, hash: %s:%d;-;.;%d
all is good
