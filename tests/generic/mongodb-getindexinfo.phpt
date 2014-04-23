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
$d->col1->ensureIndex(array('b' => '2d'), array('sparse' => true));

// check indexes
$indexes = $d->col1->getIndexInfo();
foreach($indexes as $index) {
    dump_these_keys($index, array("key", "ns", "name"));
}

?>
--EXPECTF--
array(3) {
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
array(3) {
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
array(3) {
  ["key"]=>
  array(1) {
    ["b"]=>
    string(2) "2d"
  }
  ["ns"]=>
  string(%d) "%s.col1"
  ["name"]=>
  string(4) "b_2d"
}
