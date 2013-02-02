--TEST--
Test for PHP-569: Checking for w in the connection string.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
MongoLog::setModule( MongoLog::IO | MongoLog::PARSE );
MongoLog::setLevel( MongoLog::FINE | MongoLog::INFO );
set_error_handler('foo'); function foo($a, $b) { echo $b, "\n"; };

$hostname = hostname();
$port = port();

$strings = array(
	"mongodb://$hostname:$port/?w=0",
	"mongodb://$hostname:$port/?w=1",
	"mongodb://$hostname:$port/?w=2",
	"mongodb://$hostname:$port/?w=allDCs",
	"mongodb://$hostname:$port/?w=majority",
);

$tests = array(
	array(),
	array( 'safe' => 0 ),
	array( 'safe' => 1 ),
	array( 'w' => 0 ),
	array( 'w' => 1 ),
);

foreach ( $strings as $string )
{
	echo "\nRunning string $string\n";
	$m = new MongoClient( $string );
	try
	{
		$m->selectDB(dbname())->test->remove();
	}
	catch ( Exception $e )
	{
		echo $e->getMessage(), "\n";
	}
	foreach ( $tests as $key => $test )
	{
		echo "\n- Running test $key, with options: ", json_encode( $test ), ":\n";
		try
		{
			$m->selectDB(dbname())->test->insert( array( '_id' => $key ), $test );
		}
		catch ( Exception $e )
		{
			echo $e->getMessage(), "\n";
		}
	}
}
?>
--EXPECTF--

Running string mongodb://%s/?w=0
PARSE   INFO: Parsing mongodb://%s/?w=0
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'w': 0
IO      FINE: is_gle_op: no

- Running test 0, with options: []:
IO      FINE: is_gle_op: no

- Running test 1, with options: {"safe":0}:
IO      FINE: is_gle_op: no

- Running test 2, with options: {"safe":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

- Running test 3, with options: {"w":0}:
IO      FINE: is_gle_op: no

- Running test 4, with options: {"w":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running string mongodb://%s/?w=1
PARSE   INFO: Parsing mongodb://%s/?w=1
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'w': 1
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

- Running test 0, with options: []:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

- Running test 1, with options: {"safe":0}:
IO      FINE: is_gle_op: no

- Running test 2, with options: {"safe":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

- Running test 3, with options: {"w":0}:
IO      FINE: is_gle_op: no

- Running test 4, with options: {"w":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running string mongodb://%s/?w=2
PARSE   INFO: Parsing mongodb://%s/?w=2
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'w': 2
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=2
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 0, with options: []:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w=2
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 1, with options: {"safe":0}:
IO      FINE: is_gle_op: no

- Running test 2, with options: {"safe":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

- Running test 3, with options: {"w":0}:
IO      FINE: is_gle_op: no

- Running test 4, with options: {"w":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body

Running string mongodb://%s/?w=allDCs
PARSE   INFO: Parsing mongodb://%s/?w=allDCs
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'w': 'allDCs'
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='allDCs'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 0, with options: []:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='allDCs'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 1, with options: {"safe":0}:
IO      FINE: is_gle_op: no

- Running test 2, with options: {"safe":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='allDCs'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 3, with options: {"w":0}:
IO      FINE: is_gle_op: no

- Running test 4, with options: {"w":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='allDCs'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

Running string mongodb://%s/?w=majority
PARSE   INFO: Parsing mongodb://%s/?w=majority
PARSE   INFO: - Found node: %s:%d
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'w': 'majority'
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='majority'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 0, with options: []:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='majority'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 1, with options: {"safe":0}:
IO      FINE: is_gle_op: no

- Running test 2, with options: {"safe":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='majority'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Running test 3, with options: {"w":0}:
IO      FINE: is_gle_op: no

- Running test 4, with options: {"w":1}:
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='majority'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work
