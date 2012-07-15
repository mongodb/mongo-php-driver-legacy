--TEST--
bson_encode() null
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
var_dump('' === bson_encode(null));
?>
--EXPECT--
bool(true)
