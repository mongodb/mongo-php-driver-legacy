--TEST--
MongoCollection::insert() encodes arrays as objects
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'insert');
$coll->drop();

$coll->insert(array('foo', 'bar'));
$result = $coll->findOne();
var_dump($result['0']);
var_dump($result['1']);

$coll->drop();

$coll->insert(array(-2147483647 => 'xyz'));
$result = $coll->findOne();
var_dump($result['-2147483647']);
?>
--EXPECT--
string(3) "foo"
string(3) "bar"
string(3) "xyz"
