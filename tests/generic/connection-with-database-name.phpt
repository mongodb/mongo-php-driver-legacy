--TEST--
Connection strings: with database name
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";
mongo("phpunit");
mongo("bar/baz");
mongo("/");
?>
--EXPECTF--
