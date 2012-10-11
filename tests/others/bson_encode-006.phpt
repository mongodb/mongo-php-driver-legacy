--TEST--
bson_encode() array
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
// BSON document: length
$expected = pack('V', 23);

// element: UTF-8 string
$expected .= pack('Ca*xVa*x', 2, '0', 7, 'foobar');

// element: boolean
$expected .= pack('Ca*xC', 8, '1', 1);

// BSON document: end
$expected .= pack('x');

var_dump($expected === bson_encode(array('foobar', true)));
?>
--EXPECT--
bool(true)
