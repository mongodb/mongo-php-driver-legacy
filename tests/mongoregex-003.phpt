--TEST--
MongoRegex insertion
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongoregex');
$coll->drop();

$regex = new MongoRegex('/foo[bar]{3}/imx');

$coll->insert(array('_id' => 1, 'regex' => $regex));
$result = $coll->findOne(array('_id' => 1));
echo get_class($result['regex']) . "\n";
var_dump($result['regex']->regex === $regex->regex);
var_dump($result['regex']->flags === $regex->flags);
?>
--EXPECT--
MongoRegex
bool(true)
bool(true)
