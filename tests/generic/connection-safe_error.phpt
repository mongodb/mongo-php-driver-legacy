--TEST--
Connection strings: safe (errors)
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$hostname = hostname();
$port = port();

$tests = array(
	'fireAndForget=False',
	'fireAndForget=True',
	'fireAndForget=0',
	'fireAndForget=1',
	'fireAndForget=-1',
);

foreach ( $tests as $key => $value )
{
	$dsn = "mongodb://$hostname:$port/?{$value}";
	echo "\nNow testing $dsn\n";
	try
	{
		$m = new Mongo( $dsn );
		echo "OK\n";
	}
	catch ( MongoException $e )
	{
		echo "FAIL: ", $e->getMessage(), "\n";
	}
}
?>
--EXPECTF--
Now testing mongodb://%s:%d/?fireAndForget=False
FAIL: The value of 'fireAndForget' needs to be either 'true' or 'false'

Now testing mongodb://%s:%d/?fireAndForget=True
FAIL: The value of 'fireAndForget' needs to be either 'true' or 'false'

Now testing mongodb://%s:%d/?fireAndForget=0
FAIL: The value of 'fireAndForget' needs to be either 'true' or 'false'

Now testing mongodb://%s:%d/?fireAndForget=1
FAIL: The value of 'fireAndForget' needs to be either 'true' or 'false'

Now testing mongodb://%s:%d/?fireAndForget=-1
FAIL: The value of 'fireAndForget' needs to be either 'true' or 'false'
