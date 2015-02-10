--TEST--
Test for PHP-1390: hasNext() should return false when there is an empty result
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$d = $m->selectDb(dbname());
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$cursor = $c->find();

var_dump($cursor->hasNext());
var_dump($cursor->getNext());
var_dump($cursor->hasNext());

?>
--EXPECT--
bool(false)
NULL
bool(false)
