--TEST--
"mongo.native_long" INI option has no effect on 32-bit integers
--SKIPIF--
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'mongo_native_long');
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
