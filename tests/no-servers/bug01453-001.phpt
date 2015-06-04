--TEST--
Test for PHP-1450: bson_to_zval() should check that regex strings do not exceed buffer
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

function assertInvalid($data) {
    try {
        bson_decode($data);
    } catch (MongoException $e) {
        printf("%s: %s\n", get_class($e), $e->getMessage());
    }
}

echo "Regex pattern string is not null terminated within document length\n";
assertInvalid("\x0b\x00\x00\x00\x0bfoo\x00bar\x00\x00\x00");

echo "\nRegex pattern string is not null terminated within buffer length\n";
assertInvalid("\x0c\x00\x00\x00\x0bfoo\x00bar");

echo "\nRegex pattern string is null terminated within document length\n";
var_dump(bson_decode("\x0f\x00\x00\x00\x0bfoo\x00bar\x00\x00\x00"));

echo "\nRegex flags string is not null terminated within document length\n";
assertInvalid("\x0c\x00\x00\x00\x0bfoo\x00\x00bar\x00\x00");

echo "\nRegex flags string is not null terminated within buffer length\n";
assertInvalid("\x0d\x00\x00\x00\x0bfoo\x00\x00bar");

echo "\nRegex flags string is null terminated within document length\n";
var_dump(bson_decode("\x0f\x00\x00\x00\x0bfoo\x00\x00bar\x00\x00"));

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Regex pattern string is not null terminated within document length
MongoCursorException: Reading data for type 0b would exceed buffer for key "foo"

Regex pattern string is not null terminated within buffer length
MongoCursorException: Reading data for type 0b would exceed buffer for key "foo"

Regex pattern string is null terminated within document length
array(1) {
  ["foo"]=>
  object(MongoRegex)#%d (2) {
    ["regex"]=>
    string(3) "bar"
    ["flags"]=>
    string(0) ""
  }
}

Regex flags string is not null terminated within document length
MongoCursorException: Reading data for type 0b would exceed buffer for key "foo"

Regex flags string is not null terminated within buffer length
MongoCursorException: Reading data for type 0b would exceed buffer for key "foo"

Regex flags string is null terminated within document length
array(1) {
  ["foo"]=>
  object(MongoRegex)#%d (2) {
    ["regex"]=>
    string(0) ""
    ["flags"]=>
    string(3) "bar"
  }
}
===DONE===
