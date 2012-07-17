--TEST--
MongoDBRef::get() returns null if reference parameter is invalid
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB('test');

var_dump(MongoDBRef::get($db, null));
var_dump(MongoDBRef::get($db, array()));
var_dump(MongoDBRef::get($db, array('$ref' => 'dbref')));
var_dump(MongoDBRef::get($db, array('$id' => 123)));
?>
--EXPECT--
NULL
NULL
NULL
NULL
