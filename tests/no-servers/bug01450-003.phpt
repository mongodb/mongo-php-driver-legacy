--TEST--
Test for PHP-1450: bson_to_zval() should check that namespace is null-terminated
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

function createDocument($elementList, $len = null)
{
    if ($len === null) {
        // Default: int32 size + element list size + terminating null byte
        $len = 4 + strlen($elementList) + 1;
    }

    $bson  = pack('V', $len);                  // int32: document length
    $bson .= $elementList;                     // byte*: element list
    $bson .= pack('x');                        // null byte: document terminator

    return $bson;
}

function createDBPointerElement($name, $ns, $oid, $isNullTerminated = true)
{
    // string size + terminating null byte
    $nsLen = strlen($ns) + ($isNullTerminated ? 1 : 0);

    $bson  = pack('C', 0x0C);                  // byte: field type
    $bson .= pack('a*x', $name);               // cstring: field name
    $bson .= pack('V', $nsLen);                // int32: namespace length
    $bson .= pack('a*', $ns);                  // cstring: namespace characters
    $bson .= $isNullTerminated ? "\x00" : '';  // cstring: terminating null byte
    $bson .= pack('H*', $oid);                 // byte*: ObjectId bytes

    return $bson;
}

echo "Testing valid document:\n";

$oid = '55563fd89098e4d9923d0ee1';
$bson = createDocument(createDBPointerElement('foo', 'db.coll', $oid));

var_dump(bson_decode($bson));

echo "\nTesting invalid document:\n";

$bson = createDocument(createDBPointerElement('foo', 'db.coll', $oid, false));

try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Testing valid document:
array(1) {
  ["foo"]=>
  array(2) {
    ["$ref"]=>
    string(7) "db.coll"
    ["$id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "55563fd89098e4d9923d0ee1"
    }
  }
}

Testing invalid document:
MongoCursorException: DBPointer namespace string for key "foo" is not null-terminated
===DONE===
