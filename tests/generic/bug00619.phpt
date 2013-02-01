--TEST--
Test for PHP-619: Connection failure
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn, array("db" => dbname()));
?>
I'm Alive
==DONE==
--EXPECTF--
I'm Alive
==DONE==
