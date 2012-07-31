--TEST--
Array input for bson_encode()
--CREDITS--
Stefan Koopmanschap <stefan.koopmanschap@gmail.com>
# PFZ.nl/AmsterdamPHP TestFest 2012-06-23
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
// numeric keys
$input = array('test', 'foo', 'bar');
$output = bson_encode($input);
$testValue = bson_decode($output);
var_dump($testValue);

// string keys
$input = array('test' => 'test', 'foo' => 'foo', 'bar' => 'bar');
$output = bson_encode($input);
$testValue = bson_decode($output);
var_dump($testValue);

// mixed keys
$input = array('foo' => 'test', 'foo', 'bar');
$output = bson_encode($input);
$testValue = bson_decode($output);
var_dump($testValue);
?>
--EXPECTF--
array(3) {
  [0]=>
  string(4) "test"
  [1]=>
  string(3) "foo"
  [2]=>
  string(3) "bar"
}
array(3) {
  ["test"]=>
  string(4) "test"
  ["foo"]=>
  string(3) "foo"
  ["bar"]=>
  string(3) "bar"
}
array(3) {
  ["foo"]=>
  string(4) "test"
  [0]=>
  string(3) "foo"
  [1]=>
  string(3) "bar"
}
