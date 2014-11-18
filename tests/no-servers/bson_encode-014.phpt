--TEST--
bson_encode() MongoMaxKey and ongoMinKey
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
var_dump('' === bson_encode(new MongoMaxKey));
var_dump('' === bson_encode(new MongoMinKey));
?>
--EXPECT--
bool(true)
bool(true)
