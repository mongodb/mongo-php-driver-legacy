--TEST--
Test for PHP-522_error: Checking error conditions in insert options
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = mongo();
$c = $m->selectCollection( dbname(), "php-522_error" );

$c->w = "1";

try {
	$c->insert( array( 'test' => 1 ), array( 'safe' => 1 ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "DONE";
?>
--EXPECTF--
DONE
