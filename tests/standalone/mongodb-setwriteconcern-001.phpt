--TEST--
MongoDB::setWriteConcern() and MongoDB::getWriteConcern()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('w' => 'majority', 'wTimeoutMS' => 400));

$db = $mc->selectDB(dbname());

var_dump($db->getWriteConcern());
var_dump($db->setWriteConcern(0));
var_dump($db->getWriteConcern());
var_dump($db->setWriteConcern(1, 1000));
var_dump($db->getWriteConcern());
var_dump($db->setWriteConcern('majority'));
var_dump($db->getWriteConcern());

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
