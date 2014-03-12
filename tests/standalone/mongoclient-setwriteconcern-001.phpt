--TEST--
MongoClient::setWriteConcern() and MongoClient::getWriteConcern()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('w' => 'majority', 'wTimeoutMS' => 400));

var_dump($mc->getWriteConcern());
var_dump($mc->setWriteConcern(0));
var_dump($mc->getWriteConcern());
var_dump($mc->setWriteConcern(1, 1000));
var_dump($mc->getWriteConcern());
var_dump($mc->setWriteConcern('majority'));
var_dump($mc->getWriteConcern());

?>
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
