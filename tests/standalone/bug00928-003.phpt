--TEST--
Test for PHP-928: The 'w' property is read-only (out-of-scope)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

class MyDb extends MongoDB {
	function __construct($conn, $name)
	{
		parent::__construct($conn, $name);
	}
}

$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);

$db = new MyDB($m, dbname());
$db->w = 42;
echo get_class($db);
?>
--EXPECTF--
%s: main(): The 'w' property is read-only in %sbug00928-003.php on line %d
MyDb
