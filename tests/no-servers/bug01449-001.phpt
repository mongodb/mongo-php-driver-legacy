--TEST--
Test for PHP-1449: bson_to_zval() memory leak after unsupported type exception
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

function createStringElement($name, $value)
{
    // string size + terminating null byte
    $len = strlen($value) + 1;

    $bson  = pack('C', 2);                     // byte: field type
    $bson .= pack('a*x', $name);               // cstring: field name
    $bson .= pack('V', $len);                  // int32: string length
    $bson .= pack('a*x', $value);              // cstring: string

    return $bson;
}

function createUnsupportedElement($name)
{
    $bson  = pack('C', 254);                   // byte: unsupported field type
    $bson .= pack('a*x', $name);               // cstring: field name

    return $bson;
}

echo "Testing valid document:\n";

$bson = createDocument(createStringElement('foo', 'bar'));

var_dump(bson_decode($bson));

echo "\nTesting invalid document:\n";

$bson = createDocument(createUnsupportedElement('foo'));

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
MongoException: Detected unknown BSON type 0xfe for fieldname "foo". If this is an unsupported type and not data corruption, consider upgrading to the "mongodb" extension. BSON buffer: 0a 00 00 00 fe 66 6f 6f 00
===DONE===
