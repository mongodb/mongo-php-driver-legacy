--TEST--
MongoDB::setWriteConcern() emits warning if $w arg is not integer or string
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDB(dbname());

$db->setWriteConcern(1.0);
$db->setWriteConcern(array());
$db->setWriteConcern(false);

?>
===DONE===
--EXPECTF--
Warning: MongoDB::setWriteConcern(): expects parameter 1 to be an string or integer, double given in %s on line %d

Warning: MongoDB::setWriteConcern(): expects parameter 1 to be an string or integer, array given in %s on line %d

Warning: MongoDB::setWriteConcern(): expects parameter 1 to be an string or integer, boolean given in %s on line %d
===DONE===
