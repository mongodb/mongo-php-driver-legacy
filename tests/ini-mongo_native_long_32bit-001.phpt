--TEST--
Enabled "mongo.native_long" INI option throws exception reading 64-bit integer
--SKIPIF--
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongo_native_long');
$coll->drop();

ini_set('mongo.long_as_object', false);
ini_set('mongo.native_long', true);

$coll->insert(array('int64' => new MongoInt64('9223372036854775807')));

try {
    $coll->findOne();
} catch (Exception $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}
?>
--EXPECT--
MongoCursorException: Can not natively represent the long 9223372036854775807 on this platform
