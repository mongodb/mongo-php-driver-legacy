--TEST--
Database: Dropping collections (object)
--SKIPIF--
<?php $engine = "mmapv1"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// create a collection by inserting a record
$d->dropcoltest->insert(array('foo' => 'bar'));
dump_these_keys($ns->findOne(array('name' => 'phpunit.dropcoltest')), array("name"));

// drop the collection
$d->dropCollection($d->dropcoltest);
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));

// dropping the new non-existant collection
$d->dropCollection($d->dropcoltest);
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));
?>
--EXPECTF--
array(1) {
  ["name"]=>
  string(19) "phpunit.dropcoltest"
}
NULL
NULL
