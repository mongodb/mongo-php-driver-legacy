--TEST--
Custom "mongo.cmd" INI option with modifier operations
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection('foo', 'bar');
$coll->drop();

ini_set('mongo.cmd', '!');
$coll->insert(array('_id' => 1, 'name' => 'example.com'));

$coll->update(array('_id' => 1), array('!set' => array('name' => 'google.com')));
$result = $coll->findOne();
echo $result['name'] . "\n";

ini_set('mongo.cmd', '#');
$coll->update(array('_id' => 1), array('#set' => array('name' => 'yahoo.com')));
$result = $coll->findOne();
echo $result['name'] . "\n";
?>
--EXPECT--
google.com
yahoo.com
