--TEST--
MongoInt64 constructed with strings
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
var_dump('1234567890' === (string) new MongoInt64('1234567890'));
var_dump('1234567890123456' === (string) new MongoInt64('1234567890123456'));
var_dump('123456789012345678901234567890' === (string) new MongoInt64('123456789012345678901234567890'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
