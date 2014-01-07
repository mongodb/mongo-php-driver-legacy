--TEST--
Test for PHP-605: Safe write operations return NULL instead of a boolean.
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo();
$c = $m->selectDb(dbname())->bug605;
$c->remove();
$ret = $c->insert( array( 'test' => 'one' ) );
dump_these_keys($ret, array("n", "err", "ok"));
$ret = $c->insert( array( 'test' => 'two' ), array( 'safe' => true ) );
dump_these_keys($ret, array("n", "err", "ok"));

$m = old_mongo();
$c = $m->selectDb(dbname())->bug605;
$c->remove();
var_dump( $c->insert( array( 'test' => 'one' ) ) );
var_dump( $c->insert( array( 'test' => 'two' ), array( 'w' => 0 ) ) );
$ret = $c->insert( array( 'test' => 'two' ), array( 'w' => 1 ) );
dump_these_keys($ret, array("n", "err", "ok"));

$m = new_mongo();
$c = $m->selectDb(dbname())->bug605;
$c->remove();
$ret = $c->insert( array( 'test' => 'one' ) );
dump_these_keys($ret, array("n", "err", "ok"));
$ret = $c->insert( array( 'test' => 'two' ), array( 'w' => 0 ) );
var_dump($ret);
$ret = $c->insert( array( 'test' => 'two' ), array( 'w' => 1 ) );
dump_these_keys($ret, array("n", "err", "ok"));
?>
--EXPECTF--
array(3) {
  ["n"]=>
  int(0)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}

%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %sbug00605.php on line %d
array(3) {
  ["n"]=>
  int(0)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}

%s: %s: The Mongo class is deprecated, please use the MongoClient class in %sserver.inc on line %d
bool(true)
bool(true)
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
bool(true)
array(3) {
  ["n"]=>
  int(0)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
