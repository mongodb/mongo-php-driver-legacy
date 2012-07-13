--TEST--
MongoInt32 constructed with 64-bit integer
--SKIPIF--
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongoint32');
$coll->drop();

$coll->insert(array('int32' => new MongoInt32(123456789012345)));

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', false);
$result = $coll->findOne();
var_dump($result['int32']);

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', false);
$result = $coll->findOne();
var_dump($result['int32']);

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
var_dump($result['int32']);

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
var_dump($result['int32']);
?>
--EXPECT--
int(1)
int(1)
int(1)
int(1)
