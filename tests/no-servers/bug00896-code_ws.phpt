--TEST--
Test for PHP-896: Segfault decoding BSON reads past buffer endpoint (code_ws)
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

function createCodeWithScope($len, $code, $document) {
    $bson  = pack('C', 0x0F);                      // byte: field type
    $bson .= pack('a*x', 'x');                     // cstring: field name
    $bson .= pack('V', $len);                      // int32: string and document length
    $bson .= $code;                                // Code string
    $bson .= $document;                            // Scope document
    $bson .= pack('x');                            // null byte: document terminator
    $bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

    return $bson;
}

echo "\nTesting code_ws type with valid buffer length\n";

$code = createStringElement(1, '');
$scope = createBson(0x08, 1);

var_dump(bson_decode(createCodeWithScope(5 + strlen($code) + strlen($scope), $code, $scope)));

echo "\nTesting code_ws type with invalid buffer length\n";

try {
    bson_decode(createBson(0x0F, 3));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting code_ws type with invalid code buffer length\n";

$code = createStringElement(20, '');
$scope = createBson(0x08, 1);

try {
    bson_decode(createCodeWithScope(5 + strlen($code) + strlen($scope), $code, $scope));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting code_ws type with invalid scope buffer length\n";

$code = createStringElement(1, '');
$scope = pack('Vx', 50);

try {
    bson_decode(createCodeWithScope(5 + strlen($code) + strlen($scope), $code, $scope));
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--

Testing code_ws type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoCode)#%d (2) {
    ["code"]=>
    string(0) ""
    ["scope"]=>
    array(1) {
      ["x"]=>
      bool(false)
    }
  }
}

Testing code_ws type with invalid buffer length
string(56) "Reading data for type 0f would exceed buffer for key "x""
int(21)

Testing code_ws type with invalid code buffer length
string(56) "Reading data for type 0f would exceed buffer for key "x""
int(21)

Testing code_ws type with invalid scope buffer length
string(56) "Reading data for type 0f would exceed buffer for key "x""
int(21)
