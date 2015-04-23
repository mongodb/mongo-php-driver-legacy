--TEST--
MongoCollection::findAndModify() requires query, update, and fields to be a hash
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

$document = $c->findAndModify(
  array('x' => 1),
  array('$inc' => array('x' => 1)),
  array('x' => 1),
  array('new' => true)
);

printf("Set x to %d\n", $document['x']);

$document = $c->findAndModify(
  (object) array('x' => 2),
  (object) array('$inc' => array('x' => 1)),
  (object) array('x' => 1),
  array('new' => true)
);

printf("Set x to %d\n", $document['x']);

$document = $c->findAndModify(
  1,
  array('$inc' => array('x' => 1)),
  array('x' => 1),
  array('new' => true)
);

var_dump($document);

$document = $c->findAndModify(
  array('x' => 3),
  true,
  array('x' => 1),
  array('new' => true)
);

var_dump($document);

$document = $c->findAndModify(
  array('x' => 3),
  array('$inc' => array('x' => 1)),
  'foo',
  array('new' => true)
);

var_dump($document);

?>
===DONE===
--EXPECTF--
Set x to 2
Set x to 3

Warning: MongoCollection::findAndModify() expects parameter 1 to be array, integer given in %s on line %d
NULL

Warning: MongoCollection::findAndModify() expects parameter 2 to be array, boolean given in %s on line %d
NULL

Warning: MongoCollection::findAndModify() expects parameter 3 to be array, string given in %s on line %d
NULL
===DONE===
