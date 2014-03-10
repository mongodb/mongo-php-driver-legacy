--TEST--
MongoClient::setWriteConcern() emits warning if $w arg is not integer or string
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$mc->setWriteConcern(1.0);
$mc->setWriteConcern(array());
$mc->setWriteConcern(false);

?>
===DONE===
--EXPECTF--
Warning: MongoClient::setWriteConcern(): expects parameter 1 to be an string or integer, double given in %s on line %d

Warning: MongoClient::setWriteConcern(): expects parameter 1 to be an string or integer, array given in %s on line %d

Warning: MongoClient::setWriteConcern(): expects parameter 1 to be an string or integer, boolean given in %s on line %d
===DONE===
