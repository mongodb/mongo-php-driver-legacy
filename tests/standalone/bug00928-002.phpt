--TEST--
Test for PHP-928: The 'w' property is read-only (inherited method)
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

	function setW($w)
	{
		$this->w = $w;
	}
}

$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);

$db = new MyDB($m, dbname());
echo get_class($db);
?>
--EXPECTF--
MyDb
