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

var_dump( $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'safe' => 1, 'w' => 4, 'timeout' => "45foo" ) ) );
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

try {
	$m = new Mongo("mongodb://%s/?fireAndForget=false;W=setincon;wtimeout=20000");
	$dbname = dbname();
	var_dump($m->$dbname, $m->$dbname->tests);
	$d = $m->$dbname;
	$d->w = "bar";
	$d->wtimeout = 20001;
	$c = $d->tests;
	var_dump($d, $c);
	$c->w = "baz";
	$c->wtimeout = 20002;
	var_dump($d, $c);

	$c->insert( array( "test" => "foo" ) );
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
echo "-----\n";
?>
--EXPECTF--
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
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
-----
IO      FINE: append_getlasterror: added w=2
IO      FINE: append_getlasterror: added wtimeout=10000
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=2+ won't work
-----
IO      FINE: append_getlasterror: added w=4
IO      FINE: append_getlasterror: added wtimeout=10000
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=2+ won't work
-----
IO      FINE: append_getlasterror: added w='allDCs'
IO      FINE: append_getlasterror: added wtimeout=4500
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=2+ won't work
-----
object(MongoDB)#%d (2) {
  ["w"]=>
  string(8) "setincon"
  ["wtimeout"]=>
  int(20000)
}
object(MongoCollection)#%d (2) {
  ["w"]=>
  string(8) "setincon"
  ["wtimeout"]=>
  int(20000)
}
object(MongoDB)#%d (2) {
  ["w"]=>
  string(3) "bar"
  ["wtimeout"]=>
  int(20001)
}
object(MongoCollection)#%d (2) {
  ["w"]=>
  string(3) "bar"
  ["wtimeout"]=>
  int(20001)
}
object(MongoDB)#%d (2) {
  ["w"]=>
  string(3) "bar"
  ["wtimeout"]=>
  int(20001)
}
object(MongoCollection)#%d (2) {
  ["w"]=>
  string(3) "baz"
  ["wtimeout"]=>
  int(20002)
}
IO      FINE: append_getlasterror: added w='baz'
IO      FINE: append_getlasterror: added wtimeout=20002
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=2+ won't work
-----
