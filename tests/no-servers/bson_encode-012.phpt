--TEST--
bson_encode() MongoBinData
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

// Standard binary data
$data = 'foobar';
$bin = new MongoBinData($data, MongoBinData::GENERIC);
$expected = pack('VCa*', strlen($data), MongoBinData::GENERIC, $data);
var_dump($expected === bson_encode($bin));

// Deprecated MongoBinData::BYTE_ARRAY subtype (with redundant data length)
$data = 'foobar';
$bin = new MongoBinData($data, MongoBinData::BYTE_ARRAY);
$expected = pack('VCVa*', strlen($data) + 4, MongoBinData::BYTE_ARRAY, strlen($data), $data);
var_dump($expected === bson_encode($bin));

?>
===DONE===
--EXPECT--
bool(true)
bool(true)
===DONE===
