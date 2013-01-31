--TEST--
Test for PHP-500: MongoCollection insert, update and remove no longer return booleans.
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
// Connect to mongo
$m = mongo();
$c = $m->selectCollection(dbname(), 'crash');
$c->drop();

var_dump( $c->insert( array( '_id' => 'yeah', 'value' => 'maybe' ) ) );
var_dump( $c->update( array( '_id' => 'yeah' ), array( 'value' => array( '$set' => 'yes!' ) ) ) );
var_dump( $c->remove( array( '_id' => 'yeah' ) ) );

try {
	var_dump( $c->insert( array() ) );
} catch ( Exception $e ) {
	echo "Exception: ", get_class( $e ) , ": ", $e->getMessage(), "\n";
}

var_dump( $c->insert( array( '_id' => 'yeah', 'value' => 'maybe' ) ) );
var_dump( $c->update( array( '_id' => 'yeah' ), array() ) );
var_dump( $c->findOne( array( '_id' => 'yeah' ) ) );

var_dump( $c->remove( array( '_id' => 'yeah' ) ) );
var_dump( $c->findOne( array( '_id' => 'yeah' ) ) );

var_dump( $c->update( array( '_id' => 'yeah' ), array( 'value' => array( '$set' => 'yes!' ) ) ) );
var_dump( $c->findOne( array( '_id' => 'yeah' ) ) );

var_dump( $c->insert( array( '_id' => 'yeah', 'value' => 'maybe' ), array( 'safe' => true ) ) );
var_dump( $c->update( array( '_id' => 'yeah' ), array( 'value' => array( '$set' => 'yes!' ) ), array( 'safe' => true ) ) );
var_dump( $c->remove( array( '_id' => 'yeah' ), array( 'safe' => true ) ) );
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
Exception: MongoException: no elements in doc
bool(true)
bool(true)
array(1) {
  ["_id"]=>
  string(4) "yeah"
}
bool(true)
NULL
bool(true)
NULL
array(5) {
  ["n"]=>
  int(0)
  ["lastOp"]=>
  object(MongoTimestamp)#%d (2) {
    ["sec"]=>
    int(%d)
    ["inc"]=>
    int(%d)
  }
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(6) {
  ["updatedExisting"]=>
  bool(true)
  ["n"]=>
  int(1)
  ["lastOp"]=>
  object(MongoTimestamp)#%d (2) {
    ["sec"]=>
    int(%d)
    ["inc"]=>
    int(%d)
  }
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(5) {
  ["n"]=>
  int(1)
  ["lastOp"]=>
  object(MongoTimestamp)#5 (2) {
    ["sec"]=>
    int(%d)
    ["inc"]=>
    int(%d)
  }
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
