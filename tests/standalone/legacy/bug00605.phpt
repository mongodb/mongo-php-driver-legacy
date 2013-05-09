--TEST--
Test for PHP-605: Safe write operations return NULL instead of a boolean.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectDb(dbname())->bug605;
$c->remove();
var_dump( $c->insert( array( 'test' => 'one' ) ) );
var_dump( $c->insert( array( 'test' => 'two' ), array( 'safe' => true ) ) );

$m = old_mongo_standalone();
$c = $m->selectDb(dbname())->bug605;
$c->remove();
var_dump( $c->insert( array( 'test' => 'one' ) ) );
var_dump( $c->insert( array( 'test' => 'two' ), array( 'w' => 0 ) ) );
var_dump( $c->insert( array( 'test' => 'two' ), array( 'w' => 1 ) ) );

$m = new_mongo_standalone();
$c = $m->selectDb(dbname())->bug605;
$c->remove();
var_dump( $c->insert( array( 'test' => 'one' ) ) );
var_dump( $c->insert( array( 'test' => 'two' ), array( 'w' => 0 ) ) );
var_dump( $c->insert( array( 'test' => 'two' ), array( 'w' => 1 ) ) );
?>
--EXPECTF--
%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %sbug00605.php on line %d
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}

%s: %s: The Mongo class is deprecated, please use the MongoClient class in %sserver.inc on line %d
bool(true)
bool(true)
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
bool(true)
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
