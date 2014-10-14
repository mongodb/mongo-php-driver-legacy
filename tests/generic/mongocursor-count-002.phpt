--TEST--
MongoCursor::count() with limit and skip options
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

printf("Count all documents, limit = 2, skip = 3, foundOnly = false: %d\n", $c->find()->limit(2)->skip(3)->count(false));
printf("Count all documents, limit = 2, skip = 3, foundOnly = true: %d\n", $c->find()->limit(2)->skip(3)->count(true));

?>
===DONE===
--EXPECT--
Count all documents, limit = 2, skip = 3, foundOnly = false: 4
Count all documents, limit = 2, skip = 3, foundOnly = true: 1
===DONE===
