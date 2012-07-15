--TEST--
"mongo.allow_empty_keys" INI option
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection('test', 'allow_empty_keys');
$coll->drop();

ini_set('mongo.allow_empty_keys', true);

$coll->insert(array('_id' => 1, '' => 'foo'));
$result = $coll->findOne(array('_id' => 1));
var_dump($result['']);
?>
--EXPECT--
string(3) "foo"
