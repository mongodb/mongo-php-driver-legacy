--TEST--
Test for PHP-949: ensureIndex() should not alter keys by calling to_index_string()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$c = $m->selectCollection(dbname(), 'bug00949');
$c->drop();

$keys = array('a' => true);
$retval = $c->ensureIndex($keys);
var_dump($keys);

$keys = array('b' => 1.0);
$retval = $c->ensureIndex($keys);
var_dump($keys);

$keys = array('c' => null);
$retval = $c->ensureIndex($keys);
var_dump($keys);

$keys = array('d' => array(1));
try {
    $retval = $c->ensureIndex($keys);
    echo "TEST FAILED\n";
} catch(MongoResultException $e) {
    printf("%d: %s\n", $e->getCode(), $e->getMessage());
}
var_dump($keys);

$keys = array('e' => new stdClass);
try {
    $retval = $c->ensureIndex($keys);
    echo "TEST FAILED\n";
} catch(MongoResultException $e) {
    printf("%d: %s\n", $e->getCode(), $e->getMessage());
}
var_dump($keys);

?>
===DONE===
--EXPECTF--
array(1) {
  ["a"]=>
  bool(true)
}
array(1) {
  ["b"]=>
  float(1)
}

Warning: MongoCollection::ensureIndex(): Key orderings must be scalar; null given in %s on line %d
array(1) {
  ["c"]=>
  NULL
}

Warning: MongoCollection::ensureIndex(): Key orderings must be scalar; array given in %s on line %d
67: %s:%d: bad index key pattern { d: [ 1 ] }%S
array(1) {
  ["d"]=>
  array(1) {
    [0]=>
    int(1)
  }
}

Warning: MongoCollection::ensureIndex(): Key orderings must be scalar; object given in %s on line %d
67: %s:%d: bad index key pattern { e: {} }%S
array(1) {
  ["e"]=>
  object(stdClass)#%d (0) {
  }
}
===DONE===
