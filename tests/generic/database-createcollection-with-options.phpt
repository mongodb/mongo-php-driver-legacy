--TEST--
Database: Create collection with options array
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$a = mongo();
$d = $a->selectDb(dbname());
$ns = $d->selectCollection('system.namespaces');

// cleanup
$d->dropCollection('create-col1');
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

// create
// * even though we're only setting this to 100, it allocates 1 extent, so we
//   can fit 4096, not 100, bytes of data in the collection.

$c = $d->createCollection('create-col1', array('size' => 100, 'capped' => true, 'autoIndexId' => true, 'max' => 5));
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

// check indexes
var_dump($c->getIndexInfo());

// test cap
for ($i = 0; $i < 10; $i++) {
    $c->insert(array('x' => $i), array("safe" => true));
}
foreach($c->find() as $res) {
    var_dump($res["x"]);
}
var_dump($c->count());
?>
--EXPECTF--
NULL
array(2) {
  ["name"]=>
  string(%d) "%s.create-col1"
  ["options"]=>
  array(5) {
    ["create"]=>
    string(11) "create-col1"
    ["size"]=>
    int(100)
    ["capped"]=>
    bool(true)
    ["autoIndexId"]=>
    bool(true)
    ["max"]=>
    int(5)
  }
}
array(1) {
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
    string(%d) "%s.create-col1"
    ["name"]=>
    string(4) "_id_"
  }
}
int(5)
int(6)
int(7)
int(8)
int(9)
int(5)
