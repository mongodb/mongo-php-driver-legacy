--TEST--
MongoBinData construction with default type
--FILE--
<?php
$bin = new MongoBinData('abcdefg');
var_dump($bin->bin);
var_dump($bin->type);
--EXPECT--
string(7) "abcdefg"
int(2)
