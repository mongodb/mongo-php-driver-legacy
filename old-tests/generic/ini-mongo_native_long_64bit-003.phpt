--TEST--
Disabled "mongo.native_long" INI option reads 64-bit integers as floats
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'mongo_native_long');
$coll->drop();

ini_set('mongo.native_long', true);

$coll->insert(array('int64' => 9223372036854775807));

ini_set('mongo.native_long', false);

$result = $coll->findOne();
var_dump($result['int64']);
?>
--EXPECT--
float(9.2233720368548E+18)
