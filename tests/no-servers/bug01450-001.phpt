--TEST--
Test for PHP-1450: bson_to_zval() should check that string is null-terminated
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

function createStringElement($name, $value, $isNullTerminated = true)
{
    // string size + terminating null byte
    $len = strlen($value) + ($isNullTerminated ? 1 : 0);

    $bson  = pack('C', 2);                     // byte: field type
    $bson .= pack('a*x', $name);               // cstring: field name
    $bson .= pack('V', $len);                  // int32: string length
    $bson .= pack('a*', $value);               // cstring: string characters
    $bson .= $isNullTerminated ? "\x00" : '';  // cstring: terminating null byte

    return $bson;
}

echo "Testing valid document:\n";

$bson = createDocument(createStringElement('foo', 'bar'));

var_dump(bson_decode($bson));

echo "\nTesting invalid document:\n";

$bson = createDocument(createStringElement('foo', 'bar', false));

try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
Testing valid document:
array(1) {
  ["foo"]=>
  string(3) "bar"
}

Testing invalid document:
MongoCursorException: string for key "foo" is not null-terminated
===DONE===
