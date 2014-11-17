--TEST--
MongoBinData construction with valid RFC4122 UUID
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php

$bin = new MongoBinData('abcdefghijklmnop', MongoBinData::UUID_RFC4122);
var_dump($bin->bin);
var_dump($bin->type);

?>
===DONE===
--EXPECT--
string(16) "abcdefghijklmnop"
int(4)
===DONE===
