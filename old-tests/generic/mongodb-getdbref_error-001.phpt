--TEST--
MongoDB::getDBRef() returns null if reference parameter is invalid
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB('test');

var_dump($db->getDBRef(array()));
var_dump($db->getDBRef(array('$ref' => 'dbref')));
var_dump($db->getDBRef(array('$id' => 123)));
?>
--EXPECT--
NULL
NULL
NULL
