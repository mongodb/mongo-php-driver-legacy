--TEST--
bson_encode() string
--FILE--
<?php
var_dump('foobar' === bson_encode('foobar'));
?>
--EXPECT--
bool(true)
