--TEST--
Test for PHP-522_error: Checking error conditions in insert options
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection( dbname(), "php-522_error" );

$c->w = "3";

try {
	$c->insert( array( 'test' => 1 ), array( 'safe' => 1 ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
%s:%d: norepl: no replication has been enabled, so w=%s won't work
