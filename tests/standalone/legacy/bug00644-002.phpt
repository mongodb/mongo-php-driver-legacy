--TEST--
Test for PHP-644: mongo.ping_interval and mongo.is_master_interval is unused (2)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--INI--
mongo.ping_interval=victory
mongo.is_master_interval=-5
--FILE--
<?php
echo ini_get('mongo.ping_interval'), "\n";
echo ini_get('mongo.is_master_interval'), "\n";
?>
--EXPECTF--
5
15
