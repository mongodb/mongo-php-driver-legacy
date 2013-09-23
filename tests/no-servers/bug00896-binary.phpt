--TEST--
Test for PHP-896: Segfault decoding BSON reads past buffer endpoint (binary)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
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

echo "\nTesting binary type with valid buffer length\n";

var_dump(bson_decode(createBinary(0, 0x00, '')));

echo "\nTesting binary type with invalid buffer length\n";

try {
    bson_decode(createBson(0x05, 0));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting binary type with invalid data buffer length\n";

try {
    bson_decode(createBinary(20, 0x00, ''));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--

Testing binary type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoBinData)#%d (2) {
    ["bin"]=>
    string(0) ""
    ["type"]=>
    int(0)
  }
}

Testing binary type with invalid buffer length
string(56) "Reading data for type 05 would exceed buffer for key "x""
int(21)

Testing binary type with invalid data buffer length
string(56) "Reading data for type 05 would exceed buffer for key "x""
int(21)
