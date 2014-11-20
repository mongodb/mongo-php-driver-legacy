--TEST--
bson_encode() MongoTimestamp
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

$sec = 1416266917;
$inc = 1234;
$expected = pack('V2', $inc, $sec);
var_dump($expected === bson_encode(new MongoTimestamp($sec, $inc)));

?>
===DONE===
--EXPECT--
bool(true)
===DONE===
