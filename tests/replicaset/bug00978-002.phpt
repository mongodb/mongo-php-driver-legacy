--TEST--
Test for PHP-978: Standalone connection to arbiter fails
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicasetInfo();
$arb = MongoShellServer::getAnArbiterNode();

echo "CREATING CONNECTION\n";
$m = new MongoClient("mongodb://$arb", array('replicaSet' => $rs['rsname']));
MongoLog::setModule( MongoLog::RS );
MongoLog::setLevel( MongoLog::INFO );
set_error_handler('foo'); function foo($a, $b) { echo $b, "\n"; };

echo "INSERT DATA\n";
$m->selectDb(dbname())->bug978->insert(array("test" => 1));

echo "RUNNING COMMAND\n";
$result = $m->admin->command(array('replSetGetStatus' => 1));

echo "DONE WITH COMMAND\n";
if (!array_key_exists('ok', $result) || $result['ok'] == 0) {
	echo "error\n";
	var_dump($result);
} else {
	echo "all is good\n";
}
?>
--EXPECTF--
CREATING CONNECTION
INSERT DATA
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET INFO:   - tag: dc:ny
REPLSET INFO:   - tag: server:0
RUNNING COMMAND
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
REPLSET INFO:   - tag: dc:ny
REPLSET INFO:   - tag: server:0
DONE WITH COMMAND
all is good
