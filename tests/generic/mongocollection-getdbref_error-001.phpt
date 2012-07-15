--TEST--
MongoCollection::getDBRef() returns null if reference parameter is invalid
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
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
