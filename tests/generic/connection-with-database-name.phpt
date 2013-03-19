--TEST--
Connection strings: with database name
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
mongo("phpunit");
mongo("bar/baz");
mongo("/");
?>
--EXPECTF--
