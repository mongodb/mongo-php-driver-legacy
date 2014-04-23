--TEST--
Test for PHP-522: Setting per-insert options. (streams)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php $needs = "2.5.5"; $needsOp = "lt" ?>
<?php require_once "tests/utils/replicaset.inc";?>
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
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'safe' => 1, 'w' => 4, 'socketTimeoutMS' => "1" ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'safe' => 1, 'w' => 4, 'socketTimeoutMS' => "1foo" ) );
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
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=4
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
%s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
-----
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=4
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
%s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
-----
MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
float(1)
-----
MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=5
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
%s:%d: Read timed out after reading 0 bytes, waited for 3.141000 seconds
-----
MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='allDCs'
IO      FINE: append_getlasterror: added wtimeout=4500 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d:%sunrecognized getLastError mode: allDCs
-----
