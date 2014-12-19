--TEST--
MongoDB::listCollections() with non-existent database
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDB(dbname() . '_non-existent');
$db->drop();

var_dump($db->listCollections());

?>
===DONE===
--EXPECT--
array(0) {
}
===DONE===
