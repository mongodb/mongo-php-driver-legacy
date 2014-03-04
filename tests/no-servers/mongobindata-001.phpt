--TEST--
MongoBinData construction with default type
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--INI--
display_errors=1
--FILE--
<?php
error_reporting(-1);

$bin = new MongoBinData('abcdefg');
var_dump($bin->bin);
var_dump($bin->type);
--EXPECT--
string(7) "abcdefg"
int(0)
