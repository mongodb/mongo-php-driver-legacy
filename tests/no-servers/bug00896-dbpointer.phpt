--TEST--
Test for PHP-896: Segfault decoding BSON reads past buffer endpoint (DBPointer)
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

function createStringElement($len, $bytes) {
    $bson  = pack('V', $len);                      // int32: string length
    $bson .= pack('a*x', $bytes);                  // cstring: string value

    return $bson;
}

function createDBPointer($ns, $oidLen) {
    $bson  = pack('C', 0x0C);                      // byte: field type
    $bson .= pack('a*x', 'x');                     // cstring: field name
    $bson .= $ns;                                  // Namespace string
    $bson .= pack(str_repeat('x', $oidLen));       // ObjectId bytes
    $bson .= pack('x');                            // null byte: document terminator
    $bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

    return $bson;
}

echo "\nTesting DBPointer type with valid buffer length\n";

var_dump(bson_decode(createDBPointer(createStringElement(1, ''), 12)));

echo "\nTesting DBPointer type with invalid buffer length\n";

try {
    bson_decode(createBson(0x0C, 3));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting DBPointer type with invalid namespace buffer length\n";

try {
    bson_decode(createDBPointer(createStringElement(6, ''), 0));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting DBPointer type with invalid ObjectId buffer length\n";

try {
    bson_decode(createDBPointer(createStringElement(1, ''), 6));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--

Testing DBPointer type with valid buffer length
array(1) {
  ["x"]=>
  array(2) {
    ["$ref"]=>
    string(0) ""
    ["$id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "000000000000000000000000"
    }
  }
}

Testing DBPointer type with invalid buffer length
string(56) "Reading data for type 0c would exceed buffer for key "x""
int(21)

Testing DBPointer type with invalid namespace buffer length
string(56) "Reading data for type 0c would exceed buffer for key "x""
int(21)

Testing DBPointer type with invalid ObjectId buffer length
string(56) "Reading data for type 0c would exceed buffer for key "x""
int(21)
