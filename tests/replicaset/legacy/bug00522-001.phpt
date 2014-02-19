--TEST--
Test for PHP-522: Setting per-insert options. (streams)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/replicaset.inc";?>
--INI--
error_reporting=0
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo();
$c = $m->selectCollection( dbname(), "php-522_error" );

try {
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'w' => 4, 'socketTimeoutMS' => "1" ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'w' => 4, 'socketTimeoutMS' => "1foo" ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => 1, 'socketTimeoutMS' => M_PI * 100 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "yesplease", 'safe' => 5, 'socketTimeoutMS' => M_PI * 1000 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	$c->wtimeout = 4500;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => "allDCs", 'socketTimeoutMS' => M_PI * 1000 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";
?>
--EXPECTF--
%s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
-----
%s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
-----
float(1)
-----
%s:%d: Read timed out after reading 0 bytes, waited for 3.141000 seconds
-----
%s:%d:%sunrecognized getLastError mode: allDCs
-----
