--TEST--
MongoInt32 constructed with 32-bit integer
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection('phpunit', 'mongoint32');
$coll->drop();

$coll->insert(array('int32' => new MongoInt32(1234567890)));

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', false);
$result = $coll->findOne();
var_dump($result['int32']);

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
var_dump($result['int32']);
?>
--EXPECT--
int(1234567890)
int(1234567890)
