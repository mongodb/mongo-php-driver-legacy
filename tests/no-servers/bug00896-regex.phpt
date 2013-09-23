--TEST--
Test for PHP-896: Segfault decoding BSON reads past buffer endpoint (regex)
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

echo "\nTesting regex type with valid buffer length\n";

var_dump(bson_decode(createBson(0x0B, 2)));

echo "\nTesting regex type with invalid pattern buffer length\n";

try {
    bson_decode(createBson(0x0B, 0));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting regex type with invalid options buffer length\n";

try {
    bson_decode(createBson(0x0B, 1));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--

Testing regex type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoRegex)#%d (2) {
    ["regex"]=>
    string(0) ""
    ["flags"]=>
    string(0) ""
  }
}

Testing regex type with invalid pattern buffer length
string(56) "Reading data for type 0b would exceed buffer for key "x""
int(21)

Testing regex type with invalid options buffer length
string(56) "Reading data for type 0b would exceed buffer for key "x""
int(21)
