--TEST--
MongoCollection::next() should not throw exceptions for "err" document fields
--SKIPIF--
<?php require_once 'tests/utils/standalone.inc' ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$c = $mc->selectCollection(dbname(), 'mongocollection-next-001');
$c->drop();

$c->insert(array('_id' => 1, 'err' => 'foo'));
var_dump($c->findOne(array('_id' => 1)));
?>
--EXPECTF--
array(2) {
  ["_id"]=>
  int(1)
  ["err"]=>
  string(3) "foo"
}
