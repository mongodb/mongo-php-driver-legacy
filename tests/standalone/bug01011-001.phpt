--TEST--
Test for PHP-1011: MongoDB does not inherit write concern string mode option
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('w' => 'majority'));

$db = $mc->selectDB(dbname());

dump_these_keys($db->getWriteConcern(), array('w'));

?>
===DONE===
--EXPECTF--
array(1) {
  ["w"]=>
  string(8) "majority"
}
===DONE===
