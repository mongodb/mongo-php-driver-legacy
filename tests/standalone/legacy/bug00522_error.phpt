--TEST--
Test for PHP-522: Checking error conditions in insert options
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection( dbname(), "php-522_error" );

$c->w = "3";

try {
	$c->insert( array( 'test' => 1 ), array( 'w' => 3 ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
%s:%d:%s
