--TEST--
Test for PHP-522_error: Checking error conditions in insert options
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = mongo();
$c = $m->selectCollection( dbname(), "php-522_error" );

$c->insert( array( 'test' => 1 ), array( 'fsync' => M_PI, 'safe' => M_PI, 'timeout' => "foo" ) );
echo "---------\n";
$c->insert( array( 'test' => 1 ), array( 'fsync' => M_PI, 'safe' => 1, 'timeout' => "foo" ) );
?>
--EXPECTF--
Warning: MongoCollection::insert(): The value of the 'safe' option either needs to be a boolean or a string in %sbug00522_error.php on line 7
---------

Warning: MongoCollection::insert(): The value of the 'fsync' option needs to be a boolean or an integer in %sbug00522_error.php on line 9

Warning: MongoCollection::insert(): The value of the 'timeout' option needs to be an integer in %sbug00522_error.php on line 9
