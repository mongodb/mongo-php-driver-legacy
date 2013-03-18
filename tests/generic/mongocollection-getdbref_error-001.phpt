--TEST--
MongoCollection::getDBRef() returns null if reference parameter is invalid
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'dbref');

var_dump($coll->getDBRef(array()));
var_dump($coll->getDBRef(array('$ref' => 'dbref')));
var_dump($coll->getDBRef(array('$id' => 123)));
?>
--EXPECT--
NULL
NULL
NULL
