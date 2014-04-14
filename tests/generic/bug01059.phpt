--TEST--
Test for PHP-1059: updatedExisting is always true when updating
--SKIPIF--
<?php $needs = "2.6.0"; $needsOp = "gt"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

$result = $collection->update(array('test' => 1), array('test' => 2), array('upsert' => true));
dump_these_keys($result, array("nModified", "n", "updatedExisting"));
$result = $collection->update(array('test' => 2), array('test' => 3), array('upsert' => true));
dump_these_keys($result, array("nModified", "n", "updatedExisting"));
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(3) {
  ["nModified"]=>
  int(0)
  ["n"]=>
  int(1)
  ["updatedExisting"]=>
  bool(false)
}
array(3) {
  ["nModified"]=>
  int(1)
  ["n"]=>
  int(1)
  ["updatedExisting"]=>
  bool(true)
}
===DONE===
