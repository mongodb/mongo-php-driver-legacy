--TEST--
Test for PHP-949: toIndexString() creates wrong index names
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

if (version_compare( phpversion(), '5.3', '<' )) {
  ini_set('error_reporting', E_ALL & ~E_STRICT);
} else {
  ini_set('error_reporting', E_ALL & ~E_DEPRECATED);
}

class MongoCollectionStub extends MongoCollection
{
    public function __construct() {}

    public function createIndexString($keys)
    {
      return $this->toIndexString($keys);
    }
}

$c = new MongoCollectionStub();

// Valid index orderings
var_dump($c->createIndexString(array('stringPlugin' => '2d')));
var_dump($c->createIndexString(array('intPos1' => 1)));
var_dump($c->createIndexString(array('intNeg1' => -1)));
var_dump($c->createIndexString(array('floatPos1' => 1.0)));
var_dump($c->createIndexString(array('floatNeg1' => -1.0)));

// Invalid index orderings (will be accepted by server)
var_dump($c->createIndexString(array('boolTrue' => true)));
var_dump($c->createIndexString(array('boolFalse' => false)));
var_dump($c->createIndexString(array('floatPos' => 3.14)));
var_dump($c->createIndexString(array('floatNeg' => -3.14)));
var_dump($c->createIndexString(array('floatZero' => 0.0)));
var_dump($c->createIndexString(array('intPos' => 5)));
var_dump($c->createIndexString(array('intNeg' => -5)));
var_dump($c->createIndexString(array('intZero' => 0)));
var_dump($c->createIndexString(array('null' => null)));

// Invalid index orderings (will be rejected by server)
var_dump($c->createIndexString(array('stringInvalidPlugin' => 'invalidPlugin')));
var_dump($c->createIndexString(array('array' => array(1))));
var_dump($c->createIndexString(array('object' => new stdClass)));

?>
===DONE===
--EXPECTF--
string(%d) "stringPlugin_2d"
string(%d) "intPos1_1"
string(%d) "intNeg1_-1"
string(%d) "floatPos1_1"
string(%d) "floatNeg1_-1"
string(%d) "boolTrue_1"

Notice: MongoCollection::toIndexString(): Boolean false ordering is ascending in %s on line %d
string(%d) "boolFalse_1"
string(%d) "floatPos_1"
string(%d) "floatNeg_-1"
string(%d) "floatZero_1"
string(%d) "intPos_1"
string(%d) "intNeg_-1"
string(%d) "intZero_1"

Warning: MongoCollection::toIndexString(): Key orderings must be scalar; null given in %s on line %d
string(%d) "null_1"
string(%d) "stringInvalidPlugin_invalidPlugin"

Warning: MongoCollection::toIndexString(): Key orderings must be scalar; array given in %s on line %d
string(%d) "array_1"

Warning: MongoCollection::toIndexString(): Key orderings must be scalar; object given in %s on line %d
string(%d) "object_1"
===DONE===
