--TEST--
Test for PHP-1435: MongoCollection::getIndexInfo on non-existing collection segfaults
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectCollection('doesnotexist', 'doesnotexist');
$i = $c->getIndexInfo();
$m->close();
var_dump($i);
?>
==DONE==
--EXPECTF--
array(0) {
}
==DONE==
