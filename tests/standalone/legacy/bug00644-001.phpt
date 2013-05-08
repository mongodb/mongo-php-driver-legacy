--TEST--
Test for PHP-644: mongo.ping_interval and mongo.is_master_interval is unused (1)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--INI--
mongo.ping_interval=103
mongo.is_master_interval=823
--FILE--
<?php
echo ini_get('mongo.ping_interval'), "\n";
echo ini_get('mongo.is_master_interval'), "\n";
?>
--EXPECTF--
103
823
