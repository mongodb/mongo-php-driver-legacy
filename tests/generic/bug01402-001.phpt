--TEST--
Test for PHP-1402: Fix for PHP-1394 caused hasNext() regression error from PHP-1382 when limiting to 1
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);
$coll = $mc->selectCollection(dbname(), collname(__FILE__));
$coll->remove();
$coll->save(array('_id' => 'test1'));
$cur = $coll->find(array('_id' => new MongoRegex('/^test.*$/')), array('_id' => 1))->limit(1);

while ($cur->hasNext()) {
    $arr = $cur->getNext();
    var_dump($arr['_id']);
}
?>
===DONE===
--EXPECTF--
string(5) "test1"
===DONE===
