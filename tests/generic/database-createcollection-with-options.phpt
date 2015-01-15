--TEST--
Database: Create collection with options array
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/collection-info.inc";

$dsn = MongoShellServer::getStandaloneInfo();

$a = new MongoClient($dsn);
$d = $a->selectDb(dbname());

// cleanup
$d->dropCollection('create-col1');
var_dump(findCollection($d, 'create-col1'));

$c = $d->createCollection('create-col1', array('size' => 4096, 'capped' => true, 'autoIndexId' => true, 'max' => 5));
$retval = findCollection($d, 'create-col1');
var_dump($retval['name']);
dump_these_keys($retval['options'], array('size', 'capped', 'autoIndexId', 'max'));

// check indexes
$indexes = $c->getIndexInfo();
var_dump(count($indexes));
dump_these_keys($indexes[0], array('v', 'key', 'ns'));

// test cap
for ($i = 0; $i < 10; $i++) {
    $c->insert(array('x' => $i), array("w" => true));
}
foreach($c->find() as $res) {
    var_dump($res["x"]);
}
var_dump($c->count());
?>
--EXPECT--
NULL
string(11) "create-col1"
array(4) {
  ["size"]=>
  int(4096)
  ["capped"]=>
  bool(true)
  ["autoIndexId"]=>
  bool(true)
  ["max"]=>
  int(5)
}
int(1)
array(3) {
  ["v"]=>
  int(1)
  ["key"]=>
  array(1) {
    ["_id"]=>
    int(1)
  }
  ["ns"]=>
  string(16) "test.create-col1"
}
int(5)
int(6)
int(7)
int(8)
int(9)
int(5)
