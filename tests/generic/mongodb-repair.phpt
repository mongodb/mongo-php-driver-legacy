--TEST--
MongoDB::repair() Repair a database
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);

$db = $m->selectDb(dbname());
var_dump($db->repair());
?>
===DONE===
--EXPECT--
array(1) {
  ["ok"]=>
  float(1)
}
===DONE===
