--TEST--
Test for PHP-949: ensureIndex() creates wrong names
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();


$mc = new MongoClient($host);
$mc->test->index->drop();
$retval = $mc->test->index->ensureIndex(array("x" => true));
dump_these_keys($retval, array("n", "err", "ok"));
$retval = $mc->test->index->ensureIndex(array("y" => false));
dump_these_keys($retval, array("n", "err", "ok"));
try {
    $retval = $mc->test->index->ensureIndex(array("z" => array(1,2,3)));
    echo "TEST FAILED\n";
} catch(MongoWriteConcernException $e) {
    echo $e->getCode(), ": ", $e->getMessage(), "\n";
}
try {
    $retval = $retval = $mc->test->index->ensureIndex(array("a" => 4.5));
    echo "TEST FAILED\n";
} catch(MongoWriteConcernException $e) {
    echo $e->getCode(), ": ", $e->getMessage(), "\n";
}
$indexes = $mc->test->index->getIndexInfo();
foreach($indexes as $index) {
    dump_these_keys($index, array("v", "key", "name", "ns"));
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(3) {
  ["n"]=>
  int(0)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(3) {
  ["n"]=>
  int(0)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}

Notice: Array to string conversion in %s on line %d
%S67%S: 127.0.0.1:30000: Unknown index plugin 'Array' in index { z: "Array" }
%S67%S: 127.0.0.1:30000: Unknown index plugin '4.5' in index { a: "4.5" }
array(4) {
  ["v"]=>
  int(1)
  ["key"]=>
  array(1) {
    ["_id"]=>
    int(1)
  }
  ["name"]=>
  string(4) "_id_"
  ["ns"]=>
  string(10) "test.index"
}
array(4) {
  ["v"]=>
  int(1)
  ["key"]=>
  array(1) {
    ["x"]=>
    bool(true)
  }
  ["name"]=>
  string(3) "x_1"
  ["ns"]=>
  string(10) "test.index"
}
array(4) {
  ["v"]=>
  int(1)
  ["key"]=>
  array(1) {
    ["y"]=>
    bool(false)
  }
  ["name"]=>
  string(4) "y_-1"
  ["ns"]=>
  string(10) "test.index"
}
===DONE===
