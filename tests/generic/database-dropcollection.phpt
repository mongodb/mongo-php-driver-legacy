--TEST--
Database: Dropping collections (name-as-string)
--SKIPIF--
<?php $engine = "mmapv1"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb("phpunit");
$c = $d->selectCollection("dropcoltest");
$ns = $d->selectCollection('system.namespaces');

// create a collection by inserting a record
$c->insert(array('foo' => 'bar'));
dump_these_keys($ns->findOne(array('name' => 'phpunit.dropcoltest')), array("name"));

// drop the collection
$d->dropCollection('dropcoltest');
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));

// dropping the new non-existant collection
$d->dropCollection('dropcoltest');
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));
?>
--EXPECTF--
array(1) {
  ["name"]=>
  string(19) "phpunit.dropcoltest"
}
NULL
NULL
