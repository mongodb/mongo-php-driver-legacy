--TEST--
Test for PHP-723: Possibly invalid read in MongoCollection getter
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$db = $mc->test;

$collection = $db->part1->part2;
var_dump($collection->getName());

?>
--EXPECTF--
string(11) "part1.part2"
