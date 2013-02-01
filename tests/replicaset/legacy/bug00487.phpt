--TEST--
Test for PHP-487: Connect to replicaset member using standalone connection
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new Mongo("$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT");

try {
    $c = $m->selectDb(dbname())->test;
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
string(%d) "%s:%d: not master and slaveOk=false"

%s: Function Mongo::setSlaveOkay() is deprecated in %s on line %d
bool(false)
===DONE===
