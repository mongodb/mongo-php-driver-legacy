--TEST--
Test for PHP-487: Connect to replicaset member using standalone connection
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$cfg = MongoShellServer::getReplicasetInfo();
$m = new Mongo($cfg["hosts"][2]);

try {
    $c = $m->selectDb(dbname())->test;
    $c->drop();
    var_dump($c->findOne());
} catch(Exception $e) {
    var_dump($e->getMessage());
}


$retval = $m->setSlaveOkay(true);
var_dump($retval);

$c = $m->selectDb(dbname())->test;
// We don't care about the data, just the fact we don't get an exception here
$c->findOne();
?>
===DONE===
--EXPECTF--
%s: %s(): The Mongo class is deprecated, please use the MongoClient class in %sbug00487.php on line %d
string(%d) "%s:%d: not master and slaveOk=false"

%s: Function Mongo::setSlaveOkay() is deprecated in %s on line %d
bool(false)
===DONE===
