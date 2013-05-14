--TEST--
Test for MongoDB->setWriteConcern()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDb(dbname());

var_dump($db->getWriteConcern());
var_dump($db->setWriteConcern(3, 400));
var_dump($db->getWriteConcern());
var_dump($db->setWriteConcern(0));
var_dump($db->getWriteConcern());

$c = $db->wc;
var_dump($c->getWriteConcern());
var_dump($c->setWriteConcern(1, 1000));
var_dump($c->getWriteConcern());
var_dump($c->setWriteConcern(2));
var_dump($c->getWriteConcern());
?>
--EXPECT--
array(2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
bool(true)
array(2) {
  ["w"]=>
  int(3)
  ["wtimeout"]=>
  int(400)
}
bool(true)
array(2) {
  ["w"]=>
  int(0)
  ["wtimeout"]=>
  int(400)
}
array(2) {
  ["w"]=>
  int(0)
  ["wtimeout"]=>
  int(400)
}
bool(true)
array(2) {
  ["w"]=>
  int(3)
  ["wtimeout"]=>
  int(1000)
}
bool(true)
array(2) {
  ["w"]=>
  int(3)
  ["wtimeout"]=>
  int(1000)
}

