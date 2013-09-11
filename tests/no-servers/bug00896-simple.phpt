--TEST--
Test for PHP-896: Segfault decoding BSON reads past buffer endpoint (simple)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--INI--
mongo.long_as_object=1
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

/* Test boundary conditions for simple types, which only contain an expected
 * number of bytes following the field type and name components.
 */
$tests = array(
    // typeName => [typeCode, validLen, invalidLen]
    'ObjectId'  => array(0x07, 12, 11),
    'double'    => array(0x01, 8, 7),
    'boolean'   => array(0x08, 1, 0),
    'int32'     => array(0x10, 4, 3),
    'int64'     => array(0x12, 8, 7),
    'date'      => array(0x09, 8, 7),
    'timestamp' => array(0x11, 8, 7),
);

foreach ($tests as $typeName => $_) {
    list($typeCode, $validLen, $invalidLen) = $_;

    printf("\nTesting %s type with valid buffer length\n", $typeName);

    var_dump(bson_decode(createBson($typeCode, $validLen)));

    printf("\nTesting %s type with invalid buffer length\n", $typeName);

    try {
        bson_decode(createBson($typeCode, $invalidLen));
        echo "FAILED\n";
    } catch (MongoCursorException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }

    printf("\n%s\n", str_repeat('-', 80));
}

?>
--EXPECTF--

Testing ObjectId type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "000000000000000000000000"
  }
}

Testing ObjectId type with invalid buffer length
string(%d) "Reading data for type 07 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing double type with valid buffer length
array(1) {
  ["x"]=>
  float(0)
}

Testing double type with invalid buffer length
string(%d) "Reading data for type 01 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing boolean type with valid buffer length
array(1) {
  ["x"]=>
  bool(false)
}

Testing boolean type with invalid buffer length
string(%d) "Reading data for type 08 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing int32 type with valid buffer length
array(1) {
  ["x"]=>
  int(0)
}

Testing int32 type with invalid buffer length
string(%d) "Reading data for type 10 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing int64 type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoInt64)#%d (1) {
    ["value"]=>
    string(1) "0"
  }
}

Testing int64 type with invalid buffer length
string(%d) "Reading data for type 12 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing date type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoDate)#%d (2) {
    ["sec"]=>
    int(0)
    ["usec"]=>
    int(0)
  }
}

Testing date type with invalid buffer length
string(%d) "Reading data for type 09 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------

Testing timestamp type with valid buffer length
array(1) {
  ["x"]=>
  object(MongoTimestamp)#%d (2) {
    ["sec"]=>
    int(0)
    ["inc"]=>
    int(0)
  }
}

Testing timestamp type with invalid buffer length
string(%d) "Reading data for type 11 would exceed buffer for key "x""
int(21)

--------------------------------------------------------------------------------
