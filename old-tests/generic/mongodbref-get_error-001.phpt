--TEST--
MongoDBRef::get() returns null if reference parameter is invalid
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
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
