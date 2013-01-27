--TEST--
Test for PHP-569: Overriding with ->w on the MongoDb object.
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
	0,
	1,
	2,
	3,
	"majority",
	"allDCs",
);

foreach ( $strings as $string )
{
	echo "\nRunning string $string\n";
	$m = new MongoClient( $string );
	$demo = $m->selectDB(dbname());
	try
	{
		$demo->test->remove();
	}
	catch ( Exception $e )
	{
		echo $e->getMessage(), "\n";
	}
	foreach ( $tests as $key => $test )
	{
		echo "\n- Setting w property to $test:\n";
		try
		{
			$demo->w = $test;
			$demo->test->insert( array( '_id' => $key ) );
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
PARSE	INFO: Parsing mongodb://%s/?w=0
PARSE	INFO: - Found node: %s:%d
PARSE	INFO: - Connection type: STANDALONE
PARSE	INFO: - Found option 'w': 0
IO	FINE: is_gle_op: no

- Setting w property to 0:
IO	FINE: is_gle_op: no

- Setting w property to 1:
IO	FINE: is_gle_op: no

- Setting w property to 2:
IO	FINE: is_gle_op: no

- Setting w property to 3:
IO	FINE: is_gle_op: no

- Setting w property to majority:
IO	FINE: is_gle_op: no

- Setting w property to allDCs:
IO	FINE: is_gle_op: no

Running string mongodb://%s/?w=1
PARSE	INFO: Parsing mongodb://%s/?w=1
PARSE	INFO: - Found node: %s:%d
PARSE	INFO: - Connection type: STANDALONE
PARSE	INFO: - Found option 'w': 1
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body

- Setting w property to 0:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body

- Setting w property to 1:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body

- Setting w property to 2:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=2
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 3:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=3
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to majority:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='majority'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to allDCs:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='allDCs'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

Running string mongodb://%s/?w=2
PARSE	INFO: Parsing mongodb://%s/?w=2
PARSE	INFO: - Found node: %s:%d
PARSE	INFO: - Connection type: STANDALONE
PARSE	INFO: - Found option 'w': 2
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=2
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 0:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body

- Setting w property to 1:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=2
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 2:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=2
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 3:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=3
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to majority:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='majority'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to allDCs:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='allDCs'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

Running string mongodb://%s/?w=allDCs
PARSE	INFO: Parsing mongodb://%s/?w=allDCs
PARSE	INFO: - Found node: %s:%d
PARSE	INFO: - Connection type: STANDALONE
PARSE	INFO: - Found option 'w': 'allDCs'
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='allDCs'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 0:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body

- Setting w property to 1:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='allDCs'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 2:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=2
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 3:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=3
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to majority:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='majority'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to allDCs:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='allDCs'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

Running string mongodb://%s/?w=majority
PARSE	INFO: Parsing mongodb://%s/?w=majority
PARSE	INFO: - Found node: %s:%d
PARSE	INFO: - Connection type: STANDALONE
PARSE	INFO: - Found option 'w': 'majority'
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='majority'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 0:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body

- Setting w property to 1:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='majority'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 2:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=2
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to 3:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w=3
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to majority:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='majority'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work

- Setting w property to allDCs:
IO	FINE: is_gle_op: yes
IO	FINE: append_getlasterror
IO	FINE: append_getlasterror: added w='allDCs'
IO	FINE: append_getlasterror: added wtimeout=10000
IO	FINE: getting reply
IO	FINE: getting cursor header
IO	FINE: getting cursor body
%s:%d: norepl: no replication has been enabled, so w=%s won't work
