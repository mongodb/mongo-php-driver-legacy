--TEST--
Test for PHP-896: Segfault decoding BSON reads past buffer endpoint
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

function createDBPointer($ns, $oid) {
    $bson  = pack('C', 0x0C);                      // byte: field type
    $bson .= pack('a*x', 'x');                     // cstring: field name
    $bson .= pack('V', strlen($ns) + 1);           // int32: namespace string length
    $bson .= pack('a*x', $ns);                     // cstring: namespace string value
    $bson .= pack('H*', $oid);                     // ObjectId bytes
    $bson .= pack('x');                            // null byte: document terminator
    $bson  = pack('V', 4 + strlen($bson)) . $bson; // int32: document length

    return $bson;
}

var_dump(bson_decode(createDBPointer('foo.bar', '522f2461e84df1f6378b4567')));

?>
--EXPECTF--

array(1) {
  ["x"]=>
  array(2) {
    ["$ref"]=>
    string(7) "foo.bar"
    ["$id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "522f2461e84df1f6378b4567"
    }
  }
}
