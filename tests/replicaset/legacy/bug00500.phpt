--TEST--
Test for PHP-500: MongoCollection insert, update and remove no longer return booleans.
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
// Connect to mongo
$m = old_mongo();
$c = $m->selectCollection(dbname(), 'crash');
$c->drop();

var_dump( $c->insert( array( '_id' => 'yeah', 'value' => 'maybe' ) ) );
var_dump( $c->update( array( '_id' => 'yeah' ), array( 'value' => array( '$set' => 'yes!' ) ) ) );
var_dump( $c->remove( array( '_id' => 'yeah' ) ) );

var_dump( $c->insert( array( '_id' => 'yeah', 'value' => 'maybe' ) ) );
var_dump( $c->update( array( '_id' => 'yeah' ), array() ) );
var_dump( $c->findOne( array( '_id' => 'yeah' ) ) );

var_dump( $c->remove( array( '_id' => 'yeah' ) ) );
var_dump( $c->findOne( array( '_id' => 'yeah' ) ) );

var_dump( $c->update( array( '_id' => 'yeah' ), array( '$set' => array( 'value' => 'yes!' ) ) ) );
var_dump( $c->findOne( array( '_id' => 'yeah' ) ) );

$ret = $c->insert( array( '_id' => 'yeah', 'value' => 'maybe' ), array( 'w' => true ) );
dump_these_keys($ret, array("n", "err", "ok"));
$ret = $c->update( array( '_id' => 'yeah' ), array( '$set' => array( 'value' => 'yes!' ) ), array( 'w' => true ) );
dump_these_keys($ret, array("updatedExisting", "n", "err", "ok"));
$ret = $c->remove( array( '_id' => 'yeah' ), array( 'w' => true ) );
dump_these_keys($ret, array("n", "err", "ok"));
?>
--EXPECTF--
%s: %s(): The Mongo class is deprecated, please use the MongoClient class in %sserver.inc on line %d
bool(true)
bool(true)
bool(true)
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
array(3) {
  ["n"]=>
  int(0)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(4) {
  ["updatedExisting"]=>
  bool(true)
  ["n"]=>
  int(1)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(3) {
  ["n"]=>
  int(1)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
