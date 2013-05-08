--TEST--
Test for PHP-522: Checking error conditions in insert options
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection( dbname(), "php-522_error" );

var_dump( $c->insert( array( 'test' => 1 ), array( 'fsync' => M_PI, 'safe' => M_PI, 'socketTimeoutMS' => "foo" ) ) );
?>
--EXPECTF--
Warning: MongoCollection::insert(): The value of the 'safe' option either needs to be a boolean or a string in %sbug00522_error.php on line 7

Warning: MongoCollection::insert(): The value of the 'safe' option either needs to be a integer or string in %sbug00522_error.php on line 7
array(5) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["waited"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
