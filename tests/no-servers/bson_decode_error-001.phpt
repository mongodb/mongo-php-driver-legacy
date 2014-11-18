--TEST--
bson_decode() MongoBinData with invalid RFC4122 UUID
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

function createBson($type, $len) {
    $bson  = pack('C', $type);                     // byte: field type
    $bson .= pack('a*x', 'x');                     // cstring: field name
    $bson .= pack(str_repeat('x', $len));          // null bytes (field value)
    $bson .= pack('x');                            // null byte: document terminator
    $bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

    return $bson;
}

function createBinary($len, $subtype, $bytes) {
    $bson  = pack('C', 0x05);                      // byte: field type
    $bson .= pack('a*x', 'x');                     // cstring: field name
    $bson .= pack('V', $len);                      // int32: binary length
    $bson .= pack('C', $subtype);                  // byte: binary subtype
    $bson .= pack('a*', $bytes);                   // bytes: binary data
    $bson .= pack('x');                            // null byte: document terminator
    $bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

    return $bson;
}

echo "Testing undersized binary data\n";

try {
    bson_decode(createBinary(15, 0x04, 'abcdefghijklmno'));
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

echo "\nTesting oversized binary data\n";

try {
    bson_decode(createBinary(17, 0x04, 'abcdefghijklmnopq'));
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECT--
Testing undersized binary data
exception class: MongoCursorException
exception message: RFC4122 UUID must be 16 bytes; actually: 15
exception code: 25

Testing oversized binary data
exception class: MongoCursorException
exception message: RFC4122 UUID must be 16 bytes; actually: 17
exception code: 25
===DONE===
