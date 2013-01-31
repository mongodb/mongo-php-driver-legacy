--TEST--
MongoInt64 constructed with numeric string larger than 64-bit integers
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection('phpunit', 'mongoint64');
$coll->drop();

$coll->insert(array('int64' => new MongoInt64('123456789012345678901234567890')));

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', false);
$result = $coll->findOne();
var_dump($result['int64']);

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', false);
$result = $coll->findOne();
var_dump($result['int64']);

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
printf("%s(%s)\n", get_class($result['int64']), $result['int64']);

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
printf("%s(%s)\n", get_class($result['int64']), $result['int64']);
?>
--EXPECT--
float(9.2233720368548E+18)
int(9223372036854775807)
MongoInt64(9223372036854775807)
MongoInt64(9223372036854775807)
