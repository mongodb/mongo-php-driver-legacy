--TEST--
"mongo.allow_empty_keys" INI option
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'allow_empty_keys');
$coll->drop();

ini_set('mongo.allow_empty_keys', true);

$coll->insert(array('_id' => 1, '' => 'foo'));
$result = $coll->findOne(array('_id' => 1));
var_dump($result['']);
?>
--EXPECT--
string(3) "foo"
