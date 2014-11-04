--TEST--
MongoCollection::count() with limit and skip scalar arguments
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

printf("Count all documents: %d\n", $c->count());
printf("Count all documents, limit = 1: %d\n", $c->count(array(), 1));
printf("Count all documents, limit = 1, skip = 4: %d\n", $c->count(array(), 1, 4));
printf("Count all documents, skip = 1: %d\n", $c->count(array(), 0, 1));
printf("Count documents where x >= 2: %d\n", $c->count(array('x' => array('$gte' => 2))));
printf("Count documents where x >= 2, limit = 1: %d\n", $c->count(array('x' => array('$gte' => 2)), 1));
printf("Count documents where x >= 2, limit = 1, skip = 4: %d\n", $c->count(array('x' => array('$gte' => 2)), 1, 4));
printf("Count documents where x >= 2, skip = 1: %d\n", $c->count(array('x' => array('$gte' => 2)), 0, 1));

?>
===DONE===
--EXPECT--
Count all documents: 4
Count all documents, limit = 1: 1
Count all documents, limit = 1, skip = 4: 0
Count all documents, skip = 1: 3
Count documents where x >= 2: 3
Count documents where x >= 2, limit = 1: 1
Count documents where x >= 2, limit = 1, skip = 4: 0
Count documents where x >= 2, skip = 1: 2
===DONE===
