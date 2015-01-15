--TEST--
Test for PHP-1378: MongoDB::getCollectionInfo() returns a numerically indexed array
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);
$db = $mc->selectDB(dbname());

$db->selectCollection(collname(__FILE__))->drop();
$db->createCollection(collname(__FILE__));

$collections = $db->getCollectionInfo(array('filter' => array('name' => collname(__FILE__))));

var_dump($collections[0]['name'] == collname(__FILE__));

?>
===DONE===
--EXPECT--
bool(true)
===DONE===
