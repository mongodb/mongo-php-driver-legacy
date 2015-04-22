--TEST--
MongoCollection::distinct() requires query to be a hash
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->insert(array('x' => 1));
$c->insert(array('x' => 2));
$c->insert(array('x' => 2));
$c->insert(array('x' => 3));

echo json_encode($c->distinct('x', array())), "\n";
echo json_encode($c->distinct('x', new stdClass)), "\n";
echo json_encode($c->distinct('x', 1)), "\n";

?>
===DONE===
--EXPECTF--
[1,2,3]
[1,2,3]

Warning: MongoCollection::distinct() expects parameter 2 to be array, integer given in %s on line %d
null
===DONE===
