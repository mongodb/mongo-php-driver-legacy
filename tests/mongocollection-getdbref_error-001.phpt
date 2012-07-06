--TEST--
MongoCollection::getDBRef() returns null if reference parameter is invalid
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'dbref');

var_dump($coll->getDBRef(null));
var_dump($coll->getDBRef(array()));
var_dump($coll->getDBRef(array('$ref' => 'dbref')));
var_dump($coll->getDBRef(array('$id' => 123)));
?>
--EXPECT--
NULL
NULL
NULL
NULL
