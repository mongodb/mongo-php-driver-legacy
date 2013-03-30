--TEST--
Test for PHP-522: Setting per-insert options.
--SKIPIF--
<?php require_once "tests/utils/replication.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
MongoLog::setModule( MongoLog::IO );
MongoLog::setLevel( MongoLog::FINE );
set_error_handler( 'error' );
function error( $a, $b, $c )
{
	echo $b, "\n";
}

$m = mongo();
$c = $m->selectCollection( dbname(), "php-522_error" );

try {
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'safe' => 1, 'w' => 4, 'timeout' => "45foo" ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => 1, 'timeout' => M_PI ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "yesplease", 'safe' => 4, 'timeout' => M_PI * 1000 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	$c->wtimeout = 4500;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => "allDCs", 'timeout' => M_PI * 1000 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";
?>
--EXPECTF--
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=4
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
float(1)
-----
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
float(1)
-----
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=4
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
float(1)
-----
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='allDCs'
IO      FINE: append_getlasterror: added wtimeout=4500 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: exception: unrecognized getLastError mode: allDCs
-----
