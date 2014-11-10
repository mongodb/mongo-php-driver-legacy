--TEST--
Mongo::selectDB()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDB('db');

echo get_class($db), "\n";
echo (string) $db, "\n";
?>
--EXPECT--
MongoDB
db
