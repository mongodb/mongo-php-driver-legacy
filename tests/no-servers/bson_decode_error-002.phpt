--TEST--
bson_decode() invalid document lengths
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

echo "Document length is one byte less than smallest document\n";
assertInvalid("\x04\x00\x00\x00\x00");

echo "\nDocument does not end in null byte\n";
assertInvalid("\x05\x00\x00\x00\x01");

echo "\nBSON buffer is one byte less than smallest document\n";
assertInvalid("\x05\x00\x00\x00");

echo "\nBSON buffer contains an extra, trailing null byte\n";
assertInvalid("\x05\x00\x00\x00\x00\x00");

echo "\nDocument is missing null byte after last field\n";
assertInvalid("\x09\x00\x00\x00\x0afoo\x00");

echo "\nDocument length only covers the key's null terminator byte\n";
assertInvalid("\x07\x00\x00\x00\x02a\x00\x78\x56\x34\x12");

echo "\nDocument length is too small and string length is too large\n";
assertInvalid("\x08\x00\x00\x00\x02a\x00\x78\x56\x34\x12");

echo "\nInteger data is one byte but we expected four bytes\n";
assertInvalid("\x09\x00\x00\x00\x10a\x00\x05\x00");

echo "\nBSON buffer is a series of null bytes\n";
assertInvalid("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");

echo "\nDocument length is one byte more than expected\n";
assertInvalid("\x13\x00\x00\x00\x02foo\x00\x04\x00\x00\x00bar\x00\x00");

echo "\nEmbedded document length is one byte more than expected\n";
assertInvalid("\x18\x00\x00\x00\x03foo\x00\x0f\x00\x00\x00\x10bar\x00\xff\xff\xff\x7f\x00\x00");

echo "\nEmbedded document length is one byte more than expected\n";
assertInvalid("\x15\x00\x00\x00\x03foo\x00\x0c\x00\x00\x00\x08bar\x00\x01\x00\x00");

echo "\nEmbedded document length is one byte less than expected\n";
assertInvalid("\x15\x00\x00\x00\x03foo\x00\x0a\x00\x00\x00\x08bar\x00\x01\x00\x00");

echo "\nDocument length is two bytes less than expected\n";
assertInvalid("\x1c\x00\x00\x00\x03foo\x00\x12\x00\x00\x00\x02bar\x00\x05\x00\x00\x00baz\x00\x00\x00");

echo "\nString data is not null terminated\n";
assertInvalid("\x10\x00\x00\x00\x02a\x00\x04\x00\x00\x00abc\xff\x00");

?>
===DONE===
--EXPECT--
Document length is one byte less than smallest document
MongoCursorException: Document length (4 bytes) should be at least 5 (i.e. empty document)

Document does not end in null byte
MongoCursorException: Reading key name for type 01 would exceed buffer

BSON buffer is one byte less than smallest document
MongoCursorException: Reading document length would exceed buffer (4 bytes)

BSON buffer contains an extra, trailing null byte
MongoCursorException: Document length (5 bytes) is not equal to buffer (6 bytes)

Document is missing null byte after last field
MongoCursorException: Reading key name for type 0a would exceed buffer

Document length only covers the key's null terminator byte
MongoCursorException: Reading key name for type 02 would exceed buffer

Document length is too small and string length is too large
MongoCursorException: Reading data for type 02 would exceed buffer for key "a"

Integer data is one byte but we expected four bytes
MongoCursorException: Reading data for type 10 would exceed buffer for key "a"

BSON buffer is a series of null bytes
MongoCursorException: Document length (0 bytes) should be at least 5 (i.e. empty document)

Document length is one byte more than expected
MongoCursorException: Document length (19 bytes) exceeds buffer (18 bytes)

Embedded document length is one byte more than expected
MongoCursorException: Reading data for type 03 would exceed buffer for key "foo"

Embedded document length is one byte more than expected
MongoCursorException: Reading data for type 03 would exceed buffer for key "foo"

Embedded document length is one byte less than expected
MongoCursorException: Reading data for type 08 would exceed buffer for key "bar"

Document length is two bytes less than expected
MongoCursorException: Reading data for type 02 would exceed buffer for key "bar"

String data is not null terminated
MongoCursorException: string for key "a" is not null-terminated
===DONE===
