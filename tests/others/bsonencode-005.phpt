--TEST--
bson_encode() string
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
var_dump('foobar' === bson_encode('foobar'));
?>
--EXPECT--
bool(true)
