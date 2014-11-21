--TEST--
Test for PHP-1280: segfault without auth credentials
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
try {
$c = new MongoCommandCursor($m, 'a.d', array());
$c->rewind();
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
?>
==DONE==
--EXPECTF--
%s:%d: no such c%s
==DONE==
