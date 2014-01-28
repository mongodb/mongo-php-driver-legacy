--TEST--
MongoBinData type constants
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
var_dump(MongoBinData::GENERIC);
var_dump(MongoBinData::FUNC);
var_dump(MongoBinData::BYTE_ARRAY);
var_dump(MongoBinData::UUID);
var_dump(MongoBinData::UUID_RFC4122);
var_dump(MongoBinData::MD5);
var_dump(MongoBinData::CUSTOM);
?>
--EXPECT--
int(0)
int(1)
int(2)
int(3)
int(4)
int(5)
int(128)
