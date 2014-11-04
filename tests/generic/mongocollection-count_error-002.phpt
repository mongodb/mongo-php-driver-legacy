--TEST--
MongoCollection::count() supports either options hash or limit/skip scalar args
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
$c->insert(array('x' => 3));
$c->insert(array('x' => 4));

var_dump($c->count(array(), array('limit' => 3, 'skip' => 2)));
var_dump($c->count(array(), 3, 2));
var_dump($c->count(array(), array('limit' => 3), 2));

?>
===DONE===
--EXPECTF--
int(2)
int(2)

Warning: MongoCollection::count() expects at most 2 parameters, 3 given in %s on line %d
NULL
===DONE===
