--TEST--
MongoDBRef::get()
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
<?php if (isauth()) { die("skip The test suite doesn't support two databases at the moment"); } ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$coll1 = $mongo->selectCollection(dbname(), 'dbref');
$coll1->drop();
$coll1->insert(array('_id' => 123, 'x' => 'foo'));

$coll2 = $mongo->selectCollection('test2', 'dbref2');
$coll2->drop();
$coll2->insert(array('_id' => 456, 'x' => 'bar'));

$result = MongoDBRef::get($db, MongoDBRef::create('dbref', 123));
echo $result['x'] . "\n";

$result = MongoDBRef::get($db, MongoDBRef::create('dbref2', 456, 'test2'));
echo $result['x'] . "\n";
?>
--EXPECT--
foo
bar
