--TEST--
bson_encode() null
--FILE--
<?php
var_dump('' === bson_encode(null));
?>
--EXPECT--
bool(true)
