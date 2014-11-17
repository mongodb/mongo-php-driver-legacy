--TEST--
MongoBinData construction with invalid RFC4122 UUID
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php

echo "Testing undersized binary data\n";

try {
    $bin = new MongoBinData('abcdefghijklmno', MongoBinData::UUID_RFC4122);
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

echo "\nTesting oversized binary data\n";

try {
    $bin = new MongoBinData('abcdefghijklmnopq', MongoBinData::UUID_RFC4122);
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECT--
Testing undersized binary data
exception class: MongoException
exception message: RFC4122 UUID must be 16 bytes; actually: 15
exception code: 25

Testing oversized binary data
exception class: MongoException
exception message: RFC4122 UUID must be 16 bytes; actually: 17
exception code: 25
===DONE===
