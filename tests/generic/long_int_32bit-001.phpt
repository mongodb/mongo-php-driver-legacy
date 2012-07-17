--TEST--
Long integer insertion
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
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
var_dump($result['x']);
?>
--EXPECT--
float(9.2233720368548E+18)
float(9.2233720368548E+18)
float(9.2233720368548E+18)
float(9.2233720368548E+18)
