--TEST--
MongoDB::selectCollection() 
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);

$db = $m->selectDb(dbname());
$col = $db->selectCollection('selectCollection');

var_dump($col);
?>
===DONE===
--EXPECT--
object(MongoCollection)#3 (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
===DONE===
