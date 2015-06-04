--TEST--
bson_decode() invalid string lengths
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

function assertInvalid($data) {
    try {
        bson_decode($data);
    } catch (MongoException $e) {
        printf("%s: %s\n", get_class($e), $e->getMessage());
    }
}

echo "String length is 0\n";
assertInvalid("\x0c\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00");

echo "\nString length is -1\n";
assertInvalid("\x12\x00\x00\x00\x02\x00\xff\xff\xff\xfffoobar\x00\x00");

echo "\nSymbol string length is 0\n";
assertInvalid("\x0c\x00\x00\x00\x0e\x00\x00\x00\x00\x00\x00\x00");

echo "\nSymbol string length is -1\n";
assertInvalid("\x0c\x00\x00\x00\x0e\x00\xff\xff\xff\xfffoobar\x00\x00");

echo "\nDBPointer namespace string length is 0\n";
assertInvalid("\x18\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00RY\xb5j\xfa[\xd8A\xd6X]\x99\x00");

echo "\nDBPointer namespace string length is -1\n";
assertInvalid("\x1e\x00\x00\x00\x0c\x00\xff\xff\xff\xfffoobar\x00RY\xb5j\xfa[\xd8A\xd6X]\x99\x00");

echo "\nCode string length is 0\n";
assertInvalid("\x0c\x00\x00\x00\x0d\x00\x00\x00\x00\x00\x00\x00");

echo "\nCode string length is -1\n";
assertInvalid("\x0c\x00\x00\x00\x0d\x00\xff\xff\xff\xff\x00\x00");

echo "\nCode_WS code string length is 0\n";
assertInvalid("\x1c\x00\x00\x00\x0f\x00\x15\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x02\x00\x01\x00\x00\x00\x00\x00\x00");

echo "\nCode_WS code string length is -1\n";
assertInvalid("\x1c\x00\x00\x00\x0f\x00\x15\x00\x00\x00\xff\xff\xff\xff\x00\x0c\x00\x00\x00\x02\x00\x01\x00\x00\x00\x00\x00\x00");

echo "\nCode_WS scope string value length is 0\n";
assertInvalid("\x1c\x00\x00\x00\x0f\x00\x15\x00\x00\x00\x01\x00\x00\x00\x00\x0c\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00");

echo "\nCode_WS scope string value length is -1\n";
assertInvalid("\x1c\x00\x00\x00\x0f\x00\x15\x00\x00\x00\x01\x00\x00\x00\x00\x0c\x00\x00\x00\x02\x00\xff\xff\xff\xff\x00\x00\x00");

?>
===DONE===
--EXPECT--
String length is 0
MongoCursorException: invalid string length for key "": 0

String length is -1
MongoCursorException: invalid string length for key "": -1

Symbol string length is 0
MongoCursorException: invalid string length for key "": 0

Symbol string length is -1
MongoCursorException: invalid string length for key "": -1

DBPointer namespace string length is 0
MongoCursorException: invalid DBPointer namespace length for key "": 0

DBPointer namespace string length is -1
MongoCursorException: invalid DBPointer namespace length for key "": -1

Code string length is 0
MongoCursorException: invalid code length for key "": 0

Code string length is -1
MongoCursorException: invalid code length for key "": -1

Code_WS code string length is 0
MongoCursorException: invalid code length for key "": 0

Code_WS code string length is -1
MongoCursorException: invalid code length for key "": -1

Code_WS scope string value length is 0
MongoCursorException: invalid string length for key "": 0

Code_WS scope string value length is -1
MongoCursorException: invalid string length for key "": -1
===DONE===
