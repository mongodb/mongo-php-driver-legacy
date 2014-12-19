--TEST--
MongoCollection::getIndexInfo()
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectCollection(dbname(), collname(__FILE__));

$c->drop();
$c->createIndex(array('x' => -1, 'z' => 1));
$c->createIndex(array('y' => '2d'), array('sparse' => true));
$c->createIndex(array('z' => 1), array('name' => 'z_index'));

foreach($c->getIndexInfo() as $index) {
    $keys = array('key', 'ns', 'name');

    if (array_key_exists('sparse', $index)) {
        $keys[] = 'sparse';
    }

    dump_these_keys($index, $keys);
}

?>
===DONE===
--EXPECTF--
array(3) {
  ["key"]=>
  array(1) {
    ["_id"]=>
    int(1)
  }
  ["ns"]=>
  string(%d) "%s.%s"
  ["name"]=>
  string(4) "_id_"
}
array(3) {
  ["key"]=>
  array(2) {
    ["x"]=>
    int(-1)
    ["z"]=>
    int(1)
  }
  ["ns"]=>
  string(%d) "%s.%s"
  ["name"]=>
  string(8) "x_-1_z_1"
}
array(4) {
  ["key"]=>
  array(1) {
    ["y"]=>
    string(2) "2d"
  }
  ["ns"]=>
  string(%d) "%s.%s"
  ["name"]=>
  string(4) "y_2d"
  ["sparse"]=>
  bool(true)
}
array(3) {
  ["key"]=>
  array(1) {
    ["z"]=>
    int(1)
  }
  ["ns"]=>
  string(%d) "%s.%s"
  ["name"]=>
  string(7) "z_index"
}
===DONE===
