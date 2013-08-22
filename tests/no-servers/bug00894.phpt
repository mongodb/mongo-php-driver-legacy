--TEST--
Test for PHP-894: Segfault decoding BSON string whose length prefix is zero
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

echo "\nTesting string type with valid length prefix\n";

$bson  = pack('C', 0x02);                      // byte: string type
$bson .= pack('a*x', 'x');                     // cstring: field name
$bson .= pack('V', 1);                         // int32: string length (valid)
$bson .= pack('a*x', '');                      // cstring: string value
$bson .= pack('x');                            // null byte: document terminator
$bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

var_dump(bson_decode($bson));

echo "\nTesting string type\n";

$bson  = pack('C', 0x02);                      // byte: string type
$bson .= pack('a*x', 'x');                     // cstring: field name
$bson .= pack('V', 0);                         // int32: string length (invalid)
$bson .= pack('a*x', '');                      // cstring: string value
$bson .= pack('x');                            // null byte: document terminator
$bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

try {
    bson_decode($bson);
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting code type\n";

$bson  = pack('C', 0x0D);                      // byte: code type
$bson .= pack('a*x', 'x');                     // cstring: field name
$bson .= pack('V', 0);                         // int32: string length (invalid)
$bson .= pack('a*x', '');                      // cstring: string value
$bson .= pack('x');                            // null byte: document terminator
$bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

try {
    bson_decode($bson);
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting code with scope type\n";

$scope  = pack('C', 0x02);                        // byte: string type
$scope .= pack('a*x', 'x');                       // cstring: field name
$scope .= pack('V', 1);                           // int32: string length
$scope .= pack('a*x', '');                        // cstring: string value
$scope .= pack('x');                              // null byte: document terminator
$scope  = pack('V', 4 + strlen($scope)) . $scope; // int32: document length

$codews  = pack('V', 0);                             // int32: string length (invalid)
$codews .= pack('a*x', '');                          // cstring: string value
$codews .= $scope;                                   // document: scope
$codews  = pack('V', 4 + strlen($codews)) . $codews; // int32: codews length

$bson  = pack('C', 0x0F);                      // byte: code type
$bson .= pack('a*x', 'x');                     // cstring: field name
$bson .= $codews;                              // codews: code with scope
$bson .= pack('x');                            // null byte: document terminator
$bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

try {
    bson_decode($bson);
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting deprecated symbol type\n";

$bson  = pack('C', 0x0E);                      // byte: symbol type
$bson .= pack('a*x', 'x');                     // cstring: field name
$bson .= pack('V', 0);                         // int32: string length (invalid)
$bson .= pack('a*x', '');                      // cstring: string value
$bson .= pack('x');                            // null byte: document terminator
$bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

try {
    bson_decode($bson);
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting deprecated DBPointer type\n";

$bson  = pack('C', 0x0C);                      // byte: DBPointer type
$bson .= pack('a*x', 'x');                     // cstring: field name
$bson .= pack('V', 0);                         // int32: string length (invalid)
$bson .= pack('a*x', '');                      // cstring: string value
$bson .= pack('x12');                          // byte*12: ObjectId
$bson .= pack('x');                            // null byte: document terminator
$bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

try {
    bson_decode($bson);
    echo "FAILED\n";
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--

Testing string type with valid length prefix
array(1) {
  ["x"]=>
  string(0) ""
}

Testing string type
string(%d) "invalid string length for key "x": 0"
int(21)

Testing code type
string(%d) "invalid code length for key "x": 0"
int(24)

Testing code with scope type
string(%d) "invalid code length for key "x": 0"
int(24)

Testing deprecated symbol type
string(%d) "invalid string length for key "x": 0"
int(21)

Testing deprecated DBPointer type
string(%d) "invalid dbref length for key "x": 0"
int(3)
