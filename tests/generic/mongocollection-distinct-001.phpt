--TEST--
MongoCollection::distinct()
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

printf("Distinct x values: %s\n", json_encode($c->distinct('x')));
printf("Distinct x values where x >= 2: %s\n", json_encode($c->distinct('x', array('x' => array('$gte' => 2)))));

?>
===DONE===
--EXPECT--
Distinct x values: [1,2,3]
Distinct x values where x >= 2: [2,3]
===DONE===
