--TEST--
MongoDB::getIndexInfo()
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$a = new MongoClient($dsn);
$d = $a->selectDb(dbname());
$ns = $d->selectCollection('system.namespaces');

// cleanup
$d->dropCollection('col1');

$d->col1->ensureIndex(array('a' => 1));
$d->col1->ensureIndex(array('b' => '2dsphere'), array('sparse' => true));

// check indexes
var_dump($d->col1->getIndexInfo());

?>
--EXPECTF--
array(3) {
  [0]=>
  array(4) {
    ["v"]=>
    int(1)
    ["key"]=>
    array(1) {
      ["_id"]=>
      int(1)
    }
    ["ns"]=>
    string(%d) "%s.col1"
    ["name"]=>
    string(4) "_id_"
  }
  [1]=>
  array(4) {
    ["v"]=>
    int(1)
    ["key"]=>
    array(1) {
      ["a"]=>
      int(1)
    }
    ["ns"]=>
    string(%d) "%s.col1"
    ["name"]=>
    string(3) "a_1"
  }
  [2]=>
  array(5) {
    ["v"]=>
    int(1)
    ["key"]=>
    array(1) {
      ["b"]=>
      string(8) "2dsphere"
    }
    ["ns"]=>
    string(%d) "%s.col1"
    ["sparse"]=>
    bool(true)
    ["name"]=>
    string(10) "b_2dsphere"
  }
}
