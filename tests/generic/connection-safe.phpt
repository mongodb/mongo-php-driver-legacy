--TEST--
Connection strings: fireAndForget
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$hostname = hostname();
$port = port();

$tests = array(
	'fireAndForget=true',
	'fireAndForget=false',
	'fireAndForget=true',
	'fireAndForget=false',
);

foreach ( $tests as $key => $value )
{
	$dsn = "mongodb://$hostname:$port/?{$value}";
	echo "\nNow testing $dsn\n";
	$m = new Mongo( $dsn );
	$c = $m->selectCollection( dbname(), "fireAndForget-test" );
	$c->drop();
	$c->insert( array( '_id' => $key, 'value' => 'één' ) );
	try
	{
		$c->insert( array( '_id' => $key, 'value' => 'one' ) );
		echo "OK\n";
	}
	catch ( MongoException $e )
	{
		echo "FAIL: ", $e->getMessage(), "\n";
	}
}
?>
--EXPECTF--
Now testing mongodb://%s:%d/?fireAndForget=true
OK

Now testing mongodb://%s:%d/?fireAndForget=false
FAIL: %s:%d: E11000 duplicate key error index: %s.fireAndForget-test.$_id_  dup key: { : 1 }

Now testing mongodb://%s:%d/?fireAndForget=true
OK

Now testing mongodb://%s:%d/?fireAndForget=false
FAIL: %s:%d: E11000 duplicate key error index: %s.fireAndForget-test.$_id_  dup key: { : 3 }
