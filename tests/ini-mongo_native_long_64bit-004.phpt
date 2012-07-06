--TEST--
"mongo.native_long" INI option has no effect on 32-bit integers
--SKIPIF--
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongo_native_long');
$coll->drop();

ini_set('mongo.native_long', true);

$coll->insert(array('int32' => new MongoInt32(1234567890)));
$result = $coll->findOne();
var_dump($result['int32']);

$coll->drop();

ini_set('mongo.native_long', true);

$coll->insert(array('int32' => new MongoInt32(123456789012345)));
$result = $coll->findOne();
var_dump($result['int32']);
?>
--EXPECT--
int(1234567890)
int(-2045911175)
