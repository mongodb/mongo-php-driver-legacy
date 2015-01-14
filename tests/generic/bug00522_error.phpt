--TEST--
Test for PHP-522: Checking error conditions in insert options
--SKIPIF--
<?php $needs = "2.5.5"; $needsOp = "<"; ?>
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection( dbname(), "php-522_error" );

$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => M_PI, 'safe' => M_PI, 'socketTimeoutMS' => "foo" ) );
dump_these_keys($retval, array("n", "err", "ok"));
?>
--EXPECTF--
%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %sbug00522_error.php on line %d

Warning: MongoCollection::insert(): The value of the 'safe' option either needs to be a boolean or a string in %sbug00522_error.php on line 7

Warning: MongoCollection::insert(): The value of the 'safe' option either needs to be a integer or string in %sbug00522_error.php on line 7
array(3) {
  ["n"]=>
  int(0)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
