--TEST--
bson_encode() MongoBinData with invalid RFC4122 UUID
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

/* MongoBinData's properties are read-only, so use reflection to bypass
 * restrictions in our custom write handler.
 */
$rc = new ReflectionClass('MongoBinData');
$rp = $rc->getProperty('bin');

echo "Testing undersized binary data\n";

$bin = new MongoBinData('abcdefghijklmnop', MongoBinData::UUID_RFC4122);
$rp->setValue($bin, 'abcdefghijklmno');

try {
    bson_encode($bin);
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

echo "\nTesting oversized binary data\n";

$bin = new MongoBinData('abcdefghijklmnop', MongoBinData::UUID_RFC4122);
$rp->setValue($bin, 'abcdefghijklmnopq');

try {
    bson_encode($bin);
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
