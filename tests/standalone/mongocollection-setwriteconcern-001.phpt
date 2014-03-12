--TEST--
MongoCollection::setWriteConcern() and MongoCollection::getWriteConcern()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('w' => 'majority', 'wTimeoutMS' => 400));

$c = $mc->selectCollection(dbname(), collname(__FILE__));

var_dump($c->getWriteConcern());
var_dump($c->setWriteConcern(0));
var_dump($c->getWriteConcern());
var_dump($c->setWriteConcern(1, 1000));
var_dump($c->getWriteConcern());
var_dump($c->setWriteConcern('majority'));
var_dump($c->getWriteConcern());

?>
===DONE===
--EXPECT--
array(2) {
  ["w"]=>
  string(8) "majority"
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
bool(true)
array(2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(1000)
}
bool(true)
array(2) {
  ["w"]=>
  string(8) "majority"
  ["wtimeout"]=>
  int(1000)
}
===DONE===
