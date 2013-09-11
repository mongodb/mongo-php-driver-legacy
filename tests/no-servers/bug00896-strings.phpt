--TEST--
Test for PHP-896: Segfault decoding BSON reads past buffer endpoint (strings)
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

function createString($type, $len, $bytes) {
    $bson  = pack('C', $type);                     // byte: field type
    $bson .= pack('a*x', 'x');                     // cstring: field name
    $bson .= createStringElement($len, $bytes);    // string value
    $bson .= pack('x');                            // null byte: document terminator
    $bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

    return $bson;
}

/* Test boundary conditions for string-like types. These contain a length field,
 * which is also checked.
 */
$tests = array(
    // typeName => typeCode
    'string' => 0x02,
    'symbol' => 0x0E,
    'code'   => 0x0D,
);

foreach ($tests as $typeName => $typeCode) {
    printf("\nTesting %s type with valid buffer length\n", $typeName);

    var_dump(bson_decode(createString($typeCode, 1, '')));

    printf("\nTesting %s type with invalid buffer length\n", $typeName);

    try {
        bson_decode(createBson($typeCode, 3));
        echo "FAILED\n";
    } catch (MongoCursorException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }

    printf("\nTesting %s type with invalid character buffer length\n", $typeName);

    try {
        bson_decode(createString($typeCode, 6, ''));
        echo "FAILED\n";
    } catch (MongoCursorException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }

    printf("\n%s\n", str_repeat('-', 80));
}

?>
--EXPECTF--

Testing string type with valid buffer length
array(1) {
  ["x"]=>
  string(0) ""
}

Testing string type with invalid buffer length
string(56) "Reading data for type 02 would exceed buffer for key "x""
int(21)

Testing string type with invalid character buffer length
string(56) "Reading data for type 02 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing symbol type with valid buffer length
array(1) {
  ["x"]=>
  string(0) ""
}

Testing symbol type with invalid buffer length
string(56) "Reading data for type 0e would exceed buffer for key "x""
int(21)

Testing symbol type with invalid character buffer length
string(56) "Reading data for type 0e would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing code type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoCode)#%d (2) {
    ["code"]=>
    string(0) ""
    ["scope"]=>
    array(0) {
    }
  }
}

Testing code type with invalid buffer length
string(56) "Reading data for type 0d would exceed buffer for key "x""
int(21)

Testing code type with invalid character buffer length
string(56) "Reading data for type 0d would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------
