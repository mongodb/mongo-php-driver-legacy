--TEST--
Test for PHP-627: MongoConnection::aggregate() breaks on single pipeline operator argument
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection(dbname(), 'bug627');
$c->drop();

foreach (range(1,5) as $x) {
    $c->insert(array('x' => $x));
}

$group = array('$group' => array('_id' => 1, 'count' => array('$sum' => 1)));
$project = array('$project' => array('count' => 1));

$rs1 = $c->aggregate($group);
$rs2 = $c->aggregate(array($group));
$rs3 = $c->aggregate($group, $project);
$rs4 = $c->aggregate(array($group, $project));

var_dump($rs1 === $rs2);
var_dump($rs2 === $rs3);
var_dump($rs3 === $rs4);
var_dump($rs1);

--EXPECT--
bool(true)
bool(true)
bool(true)
array(2) {
  ["result"]=>
  array(1) {
    [0]=>
    array(2) {
      ["_id"]=>
      int(1)
      ["count"]=>
      int(5)
    }
  }
  ["ok"]=>
  float(1)
}
