--TEST--
bson_encode() MongoRegex
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

$pattern = '^foo.*';
$flags = 'iu';
$expected = pack('a*xa*x', $pattern, $flags);
var_dump($expected === bson_encode(new MongoRegex(sprintf('/%s/%s', $pattern, $flags))));

?>
===DONE===
--EXPECT--
bool(true)
===DONE===
