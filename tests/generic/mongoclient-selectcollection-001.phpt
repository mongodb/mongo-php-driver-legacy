--TEST--
Mongo::selectCollection()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$collection = $mc->selectCollection('db', 'collection');

echo get_class($collection), "\n";
echo (string) $collection, "\n";
?>
--EXPECT--
MongoCollection
db.collection
