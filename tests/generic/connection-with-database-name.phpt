--TEST--
Connection strings: with database name
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
mongo_standalone("phpunit");
mongo_standalone("bar/baz");
mongo_standalone("/");
?>
--EXPECTF--
