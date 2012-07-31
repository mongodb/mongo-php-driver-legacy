--TEST--
MongoDB::getDBRef() returns null if reference parameter is invalid
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB('test');

var_dump($db->getDBRef(array()));
var_dump($db->getDBRef(array('$ref' => 'dbref')));
var_dump($db->getDBRef(array('$id' => 123)));
?>
--EXPECT--
NULL
NULL
NULL
