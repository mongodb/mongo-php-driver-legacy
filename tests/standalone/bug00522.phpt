--TEST--
Test for PHP-522: Setting per-insert options.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
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
	var_dump( $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'safe' => 1, 'w' => 4, 'timeout' => "45foo" ) ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	var_dump( $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => 1, 'timeout' => M_PI ) ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	var_dump( $c->insert( array( 'test' => 1 ), array( 'fsync' => "yesplease", 'safe' => 4, 'timeout' => M_PI * 1000 ) ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";

try {
	$c->w = 2;
	$c->wtimeout = 4500;
	var_dump( $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => "allDCs", 'timeout' => M_PI * 1000 ) ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";
?>
--EXPECTF--
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=4
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: append_getlasterror: added fsync=1
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work
-----
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
-----
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=4
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: append_getlasterror: added fsync=1
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work
-----
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='allDCs'
IO	FINE: append_getlasterror: added wtimeout=4500
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work
-----
