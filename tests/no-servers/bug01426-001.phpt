--TEST--
Test for PHP-1426: bson_decode() buffer overflow via string length
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

try {
    var_dump(bson_decode("\xff\xff\xff\x10\x02\x30\x00\xff\xff\x0e\x0031337\x00\x00"));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}


try {
    var_dump(bson_decode("\xff\xff\xff\x10\x02\x30\x00\xff\xff\x10\x00XXXXX\x00\x00"));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
MongoCursorException: BSON buffer is 18 bytes, but document length is 285212671
MongoCursorException: BSON buffer is 18 bytes, but document length is 285212671
===DONE===
