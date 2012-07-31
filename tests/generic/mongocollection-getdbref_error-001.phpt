--TEST--
MongoCollection::getDBRef() returns null if reference parameter is invalid
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'dbref');

var_dump($coll->getDBRef(array()));
var_dump($coll->getDBRef(array('$ref' => 'dbref')));
var_dump($coll->getDBRef(array('$id' => 123)));
?>
--EXPECT--
NULL
NULL
NULL
