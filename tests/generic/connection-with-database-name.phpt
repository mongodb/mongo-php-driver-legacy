--TEST--
Connection strings: with database name
--FILE--
<?php
require __DIR__ . "/../utils.inc";
mongo("phpunit");
mongo("bar/baz");
mongo("/");
?>
--EXPECTF--
