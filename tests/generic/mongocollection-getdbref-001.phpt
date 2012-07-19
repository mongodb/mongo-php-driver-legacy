--TEST--
MongoCollection::getDBRef()
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
<?php if (isauth()) { die("skip The test suite doesn't support two databases at the moment"); } ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();

$coll1 = $mongo->selectCollection(dbname(), 'dbref');
$coll1->drop();
$coll1->insert(array('_id' => 123, 'x' => 'foo'));

$coll2 = $mongo->selectCollection('test2', 'dbref2');
$coll2->drop();
$coll2->insert(array('_id' => 456, 'x' => 'bar'));

$result = $coll1->getDBRef(MongoDBRef::create('dbref', 123));
echo $result['x'] . "\n";

$result = $coll1->getDBRef(MongoDBRef::create('dbref2', 456, 'test2'));
echo $result['x'] . "\n";
?>
--EXPECT--
foo
bar
