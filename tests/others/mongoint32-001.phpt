--TEST--
MongoInt32 constructed with integers
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
var_dump('1234567890' === (string) new MongoInt32(1234567890));
var_dump('1.2345678901235E+29' === (string) new MongoInt32(123456789012345678901234567890));
?>
--EXPECT--
bool(true)
bool(true)
