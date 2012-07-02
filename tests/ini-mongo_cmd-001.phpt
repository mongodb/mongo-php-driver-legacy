--TEST--
Custom "mongo.cmd" INI option with modifier operations
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
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
--EXPECTF--
google.com
yahoo.com
