--TEST--
MongoCollection::count() requires query to be a hash
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

var_dump($c->count(array()));
var_dump($c->count(new stdClass()));
var_dump($c->count(0));

?>
===DONE===
--EXPECTF--
int(4)
int(4)

Warning: MongoCollection::count() expects parameter 1 to be array, integer given in %s on line %d
NULL
===DONE===
