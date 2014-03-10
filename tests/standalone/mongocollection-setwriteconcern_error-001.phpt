--TEST--
MongoCollection::setWriteConcern() emits warning if $w arg is not integer or string
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$c = $mc->selectCollection(dbname(), collname(__FILE__));

$c->setWriteConcern(1.0);
$c->setWriteConcern(array());
$c->setWriteConcern(false);

?>
===DONE===
--EXPECTF--
Warning: MongoCollection::setWriteConcern(): expects parameter 1 to be an string or integer, double given in %s on line %d

Warning: MongoCollection::setWriteConcern(): expects parameter 1 to be an string or integer, array given in %s on line %d

Warning: MongoCollection::setWriteConcern(): expects parameter 1 to be an string or integer, boolean given in %s on line %d
===DONE===
