--TEST--
Test for PHP-569: MongoClient: Checking permutations to trigger GLE.
--SKIPIF--
<?php $needs = "2.5.5"; $needsOp = "lt"; ?>
<?php require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
MongoLog::setModule( MongoLog::IO );
MongoLog::setLevel( MongoLog::FINE );
set_error_handler('foo'); function foo($a, $b) { echo $b, "\n"; };

$m = new_mongo();
$m->selectDB(dbname())->test->remove();

$tests = array(
	array(),
	array( 'safe' => 0 ),
	array( 'safe' => 1 ),
	array( 'safe' => 2 ),
	array( 'safe' => "majority" ),
	array( 'w' => 0 ),
	array( 'w' => 1 ),
	array( 'w' => 2 ),
	array( 'w' => "majority" ),
	array( 'fsync' => 0 ),
	array( 'fsync' => 1 ),
	array( 'fsync' => 0, "w" => 1 ),
	array( 'fsync' => 1, "w" => 0 ),
);

foreach ( $tests as $key => $test )
{
	echo "\nRunning test $key, with options: ", json_encode( $test ), ":\n";
	try
	{
		$m->selectDB(dbname())->test->insert( array( '_id' => $key ), $test );
	}
	catch ( Exception $e )
	{
		echo $e->getMessage(), "\n";
	}
}
?>
--EXPECTF--
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 0, with options: []:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 1, with options: {"safe":0}:
MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead
IO      FINE: is_gle_op: no

Running test 2, with options: {"safe":1}:
MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 3, with options: {"safe":2}:
MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=2
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 4, with options: {"safe":"majority"}:
MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='majority'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 5, with options: {"w":0}:
IO      FINE: is_gle_op: no

Running test 6, with options: {"w":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 7, with options: {"w":2}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=2
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 8, with options: {"w":"majority"}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='majority'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 9, with options: {"fsync":0}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 10, with options: {"fsync":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 11, with options: {"fsync":0,"w":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running test 12, with options: {"fsync":1,"w":0}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added fsync=1
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
