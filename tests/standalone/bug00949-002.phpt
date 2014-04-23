--TEST--
Test for PHP-949: ensureIndex() creates wrong names
--SKIPIF--
<?php $needs = "2.4.0"; /* pre-2.4, index plugin strings are not validated */ ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$c = $m->selectCollection(dbname(), 'bug00949');
$c->drop();

// Valid index orderings

$c->ensureIndex(array('stringPlugin' => '2d'));
$c->ensureIndex(array('intPos1' => 1));
$c->ensureIndex(array('intNeg1' => -1));
$c->ensureIndex(array('floatPos1' => 1.0));
$c->ensureIndex(array('floatNeg1' => -1.0));

// Invalid index orderings (will be accepted by server)

$c->ensureIndex(array('boolTrue' => true));
$c->ensureIndex(array('boolFalse' => false));
$c->ensureIndex(array('floatPos' => 3.14));
$c->ensureIndex(array('floatNeg' => -3.14));
$c->ensureIndex(array('floatZero' => 0.0));
$c->ensureIndex(array('intPos' => 5));
$c->ensureIndex(array('intNeg' => -5));
$c->ensureIndex(array('intZero' => 0));
$c->ensureIndex(array('null' => null));

// Invalid index orderings (will be rejected by server)

try {
    $c->ensureIndex(array('stringInvalidPlugin' => 'invalidPlugin'));
    echo "TEST FAILED\n";
} catch(MongoResultException $e) {
    printf("%d: %s\n", $e->getCode(), $e->getMessage());
}

try {
    $c->ensureIndex(array('array' => array(1)));
    echo "TEST FAILED\n";
} catch(MongoResultException $e) {
    printf("%d: %s\n", $e->getCode(), $e->getMessage());
}

try {
    $c->ensureIndex(array('object' => new stdClass));
    echo "TEST FAILED\n";
} catch(MongoResultException $e) {
    printf("%d: %s\n", $e->getCode(), $e->getMessage());
}

foreach($c->getIndexInfo() as $index) {
    dump_these_keys($index, array('key', 'name'));
}
?>
===DONE===
--EXPECTF--
Notice: MongoCollection::ensureIndex(): Boolean false ordering is ascending in %s on line %d

Warning: MongoCollection::ensureIndex(): Key orderings must be scalar; null given in %s on line %d
67: %s:%d: %SUnknown index plugin 'invalidPlugin'%S

Warning: MongoCollection::ensureIndex(): Key orderings must be scalar; array given in %s on line %d
67: %s:%d: bad index key pattern { array: [ 1 ] }%S

Warning: MongoCollection::ensureIndex(): Key orderings must be scalar; object given in %s on line %d
67: %s:%d: bad index key pattern { object: {} }%S
array(2) {
  ["key"]=>
  array(1) {
    ["_id"]=>
    int(1)
  }
  ["name"]=>
  string(%d) "_id_"
}
array(2) {
  ["key"]=>
  array(1) {
    ["stringPlugin"]=>
    string(2) "2d"
  }
  ["name"]=>
  string(%d) "stringPlugin_2d"
}
array(2) {
  ["key"]=>
  array(1) {
    ["intPos1"]=>
    int(1)
  }
  ["name"]=>
  string(%d) "intPos1_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["intNeg1"]=>
    int(-1)
  }
  ["name"]=>
  string(%d) "intNeg1_-1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["floatPos1"]=>
    float(1)
  }
  ["name"]=>
  string(%d) "floatPos1_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["floatNeg1"]=>
    float(-1)
  }
  ["name"]=>
  string(%d) "floatNeg1_-1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["boolTrue"]=>
    bool(true)
  }
  ["name"]=>
  string(%d) "boolTrue_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["boolFalse"]=>
    bool(false)
  }
  ["name"]=>
  string(%d) "boolFalse_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["floatPos"]=>
    float(3.14)
  }
  ["name"]=>
  string(%d) "floatPos_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["floatNeg"]=>
    float(-3.14)
  }
  ["name"]=>
  string(%d) "floatNeg_-1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["floatZero"]=>
    float(0)
  }
  ["name"]=>
  string(%d) "floatZero_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["intPos"]=>
    int(5)
  }
  ["name"]=>
  string(%d) "intPos_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["intNeg"]=>
    int(-5)
  }
  ["name"]=>
  string(%d) "intNeg_-1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["intZero"]=>
    int(0)
  }
  ["name"]=>
  string(%d) "intZero_1"
}
array(2) {
  ["key"]=>
  array(1) {
    ["null"]=>
    NULL
  }
  ["name"]=>
  string(%d) "null_1"
}
===DONE===
