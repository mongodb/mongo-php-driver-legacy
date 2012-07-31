--TEST--
bson_encode() boolean
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
var_dump(chr(1) === bson_encode(true));
var_dump(chr(0) === bson_encode(false));
?>
--EXPECT--
bool(true)
bool(true)
