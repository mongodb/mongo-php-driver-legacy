--TEST--
MongoDB::getDBRef() returns null if reference parameter is invalid
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB('test');

var_dump($db->getDBRef(null));
var_dump($db->getDBRef(array()));
var_dump($db->getDBRef(array('$ref' => 'dbref')));
var_dump($db->getDBRef(array('$id' => 123)));
?>
--EXPECT--
NULL
NULL
NULL
NULL
