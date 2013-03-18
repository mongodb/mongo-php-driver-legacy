--TEST--
Long integer insertion
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'int');
$coll->drop();

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', false);
$coll->insert(array('x' => 9223372036854775807));
$result = $coll->findOne();
var_dump($result['x']);

$coll->drop();

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', false);
$coll->insert(array('x' => 9223372036854775807));
$result = $coll->findOne();
var_dump($result['x']);

$coll->drop();

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', true);
$coll->insert(array('x' => 9223372036854775807));
$result = $coll->findOne();
var_dump($result['x']);

$coll->drop();

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', true);
$coll->insert(array('x' => 9223372036854775807));
$result = $coll->findOne();
printf("%s(%s)\n", get_class($result['x']), $result['x']);
?>
--EXPECT--
int(-1)
int(9223372036854775807)
int(-1)
MongoInt64(9223372036854775807)
