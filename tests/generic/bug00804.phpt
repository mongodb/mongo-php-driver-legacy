--TEST--
Test for PHP-804: Deprecate Mongo::connectUtil.
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

class MyMongo extends Mongo
{
	public function connectUtil()
	{
		return parent::connectUtil();
	}
}

$dsn = MongoShellServer::getStandaloneInfo();

$m = new MyMongo($dsn);
$m->connectUtil();
?>
--EXPECTF--
%s: Function Mongo::connectUtil() is deprecated in %sbug00804.php on line %d
