--TEST--
MongoDBRef::get() returns null if reference parameter is invalid
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
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
