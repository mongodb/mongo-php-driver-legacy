--TEST--
Test for PHP-1109: upsert returns an array of ids rather then the _id
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

$mid = new MongoID();
$ret = $collection->update(array("_id" => $mid), array('$set' => array("x" => "y")), array("upsert" => true));
var_dump($ret["upserted"] == $mid, (string)$ret["upserted"]);
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
bool(true)
string(24) "%s"
===DONE===

