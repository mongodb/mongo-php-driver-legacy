--TEST--
Object input for bson_encode()
--CREDITS--
Stefan Koopmanschap <stefan.koopmanschap@gmail.com>
# PFZ.nl/AmsterdamPHP TestFest 2012-06-23
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

$string = '3';

$input = new stdClass();
$input->int = 3;
$input->boolean = true;
$input->array = array('foo', 'bar');
$input->object = new stdClass();
$input->string = 'test';
$input->$string = 'test';

$output = bson_encode($input);
$testValue = bson_decode($output);
var_dump($testValue);

?>
--EXPECTF--
array(6) {
  ["int"]=>
  int(3)
  ["boolean"]=>
  bool(true)
  ["array"]=>
  array(2) {
    [0]=>
    string(3) "foo"
    [1]=>
    string(3) "bar"
  }
  ["object"]=>
  array(0) {
  }
  ["string"]=>
  string(4) "test"
  [3]=>
  string(4) "test"
}
