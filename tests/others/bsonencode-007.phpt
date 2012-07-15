--TEST--
bson_encode() MongoId
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
$hex = '0123456789abcdef01234567';
$expected = pack('H*', $hex);
var_dump($expected === bson_encode(new MongoId($hex)));
?>
--EXPECT--
bool(true)
