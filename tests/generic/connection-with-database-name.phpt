--TEST--
Connection strings: with database name
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
mongo("phpunit");
mongo("bar/baz");
mongo("/");
?>
--EXPECTF--
