--TEST--
Test for PHP-978: Standalone connection to arbiter fails
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$arb = MongoShellServer::getAnArbiterNode();

echo "CREATING CONNECTION\n";
$m = new MongoClient("mongodb://$arb");
MongoLog::setModule( MongoLog::RS );
MongoLog::setLevel( MongoLog::INFO );
set_error_handler('foo'); function foo($a, $b) { echo $b, "\n"; };

echo "INSERT DATA\n";
try {
	$m->selectDb(dbname())->bug978->insert(array("test" => 1));
} catch (MongoCursorException $e) {
	echo "\n", $e->getCode(), "\n";
	echo $e->getMessage(), "\n\n";
}

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
REPLSET INFO: - connection: type: ARBITER, socket: %d, ping: %d, hash: %s:%d;-;.;%d

10%d
%s:%d: not master

RUNNING COMMAND
REPLSET INFO: pick server: random element 0
REPLSET INFO: - connection: type: ARBITER, socket: %d, ping: %d, hash: %s:%d;-;.;%d
DONE WITH COMMAND
all is good
