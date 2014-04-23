--TEST--
MongoDB::command() requires hash argument to be passed by reference
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDB(dbname());
$db->command(array('buildInfo' => 1), array(), 'foo');
?>
===DONE===
--EXPECTF--
Fatal error: Cannot pass parameter 3 by reference in %s on line %d
