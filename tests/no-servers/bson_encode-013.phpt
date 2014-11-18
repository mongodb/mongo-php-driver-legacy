--TEST--
bson_encode() MongoCode
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

// Without scope
$js = 'function() { return "foo"; }';
$expected = pack(
    'VVa*xVx',
    4 + (4 + strlen($js) + 1) + 5, // int32: code_w_s size + code string + empty scope document
    strlen($js) + 1,               // int32: code string length + null byte
    $js,                           // bytes: code string
    5                              // int32: empty scope document length
);
var_dump($expected === bson_encode(new MongoCode($js)));

// With scope
$js = 'function() { return "foo"; }';
$scope = array('bar' => true);
$expected = pack(
    'VVa*xVCa*xCx',
    4 + (4 + strlen($js) + 1) + 11, // int32: code_w_s length (size + code string + scope document)
    strlen($js) + 1,                // int32: code string length + null byte
    $js,                            // bytes: code string
    11,                             // int32: scope document length (size + key/value + null byte)
    8,                              // byte: boolean type
    'bar',                          // bytes: scope key string
    1                               // byte: boolean true value
);
var_dump($expected === bson_encode(new MongoCode($js, $scope)));

?>
===DONE===
--EXPECT--
bool(true)
bool(true)
===DONE===
