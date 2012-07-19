--TEST--
Enabled "mongo.native_long" INI option throws exception reading 64-bit integer
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'mongo_native_long');
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
