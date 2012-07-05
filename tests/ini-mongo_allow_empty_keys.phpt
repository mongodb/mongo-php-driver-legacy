--TEST--
"mongo.allow_empty_keys" INI option
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'allow_empty_keys');
$coll->drop();

ini_set('mongo.allow_empty_keys', true);

$coll->insert(array('_id' => 1, '' => 'foo'));
$result = $coll->findOne(array('_id' => 1));
var_dump($result['']);
?>
--EXPECT--
string(3) "foo"
