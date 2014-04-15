--TEST--
Test for PHP-928: The 'w' property is read-only (parent::__construct)
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
echo get_class($db);
?>
--EXPECTF--
MyDb
