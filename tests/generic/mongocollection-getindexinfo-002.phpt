--TEST--
MongoCollection::getIndexInfo() with non-existent collection
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectCollection(dbname(), collname(__FILE__));

$c->drop();

var_dump($c->getIndexInfo());

?>
===DONE===
--EXPECT--
array(0) {
}
===DONE===
