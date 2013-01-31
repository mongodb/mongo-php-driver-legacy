--TEST--
Test for PHP-522: Checking error conditions in insert options
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection( dbname(), "php-522_error" );

var_dump( $c->insert( array( 'test' => 1 ), array( 'fsync' => M_PI, 'safe' => M_PI, 'timeout' => "foo" ) ) );
?>
--EXPECTF--
Warning: MongoCollection::insert(): The value of the 'safe' option either needs to be a boolean or a string in %sbug00522_error.php on line 7
bool(true)
