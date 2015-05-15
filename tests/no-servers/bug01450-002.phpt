--TEST--
Test for PHP-1450: bson_to_zval() should check that code string is null-terminated
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

function createCodeElement($name, $code, $isNullTerminated = true)
{
    // string size + terminating null byte
    $codeLen = strlen($code) + ($isNullTerminated ? 1 : 0);

    $bson  = pack('C', 0x0D);                  // byte: field type
    $bson .= pack('a*x', $name);               // cstring: field name
    $bson .= pack('V', $codeLen);              // int32: code length
    $bson .= pack('a*', $code);                // cstring: code characters
    $bson .= $isNullTerminated ? "\x00" : '';  // cstring: terminating null byte

    return $bson;
}

echo "Testing valid document:\n";

$bson = createDocument(createCodeElement('foo', 'function(){ return 0; }'));

var_dump(bson_decode($bson));

echo "\nTesting invalid document:\n";

$bson = createDocument(createCodeElement('foo', 'function(){ return 0; }', false));

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
    array(0) {
    }
  }
}

Testing invalid document:
MongoCursorException: code string for key "foo" is not null-terminated
===DONE===
