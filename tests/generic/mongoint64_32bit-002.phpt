--TEST--
MongoInt64 constructed with 64-bit integer
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection('phpunit', 'mongoint64');
$coll->drop();

$coll->insert(array('int64' => new MongoInt64(123456789012345)));

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', false);
$result = $coll->findOne();
printf("%s\n", $result['int64']);

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
printf("%s(%s)\n", get_class($result['int64']), $result['int64']);
?>
--EXPECT--
1
MongoInt64(1)
