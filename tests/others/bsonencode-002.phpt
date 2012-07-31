--TEST--
bson_encode() integer
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
$expected = chr(123) . str_repeat(chr(0), PHP_INT_SIZE - 1);
var_dump($expected === bson_encode(123));
?>
--EXPECT--
bool(true)
