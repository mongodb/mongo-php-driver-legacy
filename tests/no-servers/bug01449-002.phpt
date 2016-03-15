--TEST--
Test for PHP-1449: bson_to_zval() memory leak after code scope decoding exception
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

function createCodeWithScopeElement($name, $code, $scope)
{
    // string size + terminating null byte
    $codeLen = strlen($code) + 1;
    // int32 + string (i.e. int32 + cstring) + scope document
    $codeWithScopeLen = 4 + (4 + $codeLen) + strlen($scope);

    $bson  = pack('C', 0x0F);                  // byte: field type
    $bson .= pack('a*x', $name);               // cstring: field name
    $bson .= pack('V', $codeWithScopeLen);     // int32: codeWithScope length
    $bson .= pack('V', $codeLen);              // int32: code length
    $bson .= pack('a*x', $code);               // cstring: code
    $bson .= $scope;                           // byte*: scope document

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
    $bson  = pack('C', 0xFE);                  // byte: unsupported field type
    $bson .= pack('a*x', $name);               // cstring: field name

    return $bson;
}

echo "Testing valid document:\n";

$bson = createDocument(
    createCodeWithScopeElement(
        'foo',
        'function(){ return 0; }',
        createDocument(
            createStringElement('bar', 'abc')
        )
    )
);

var_dump(bson_decode($bson));

echo "\nTesting invalid document:\n";

$bson = createDocument(
    createCodeWithScopeElement(
        'foo',
        'function(){ return 0; }',
        createDocument(
            createUnsupportedElement('bar')
        )
    )
);

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
  object(MongoCode)#%d (2) {
    ["code"]=>
    string(23) "function(){ return 0; }"
    ["scope"]=>
    array(1) {
      ["bar"]=>
      string(3) "abc"
    }
  }
}

Testing invalid document:
MongoException: Detected unknown BSON type 0xfe for fieldname "bar". If this is an unsupported type and not data corruption, consider upgrading to the "mongodb" extension. BSON buffer: 0a 00 00 00 fe 62 61 72 00
===DONE===
