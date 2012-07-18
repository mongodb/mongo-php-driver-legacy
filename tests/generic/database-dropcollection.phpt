--TEST--
Database: Dropping collections (name-as-string)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";
$a = mongo();
$d = $a->selectDb("phpunit");
$c = $d->selectCollection("dropcoltest");
$ns = $d->selectCollection('system.namespaces');

// create a collection by inserting a record
$c->insert(array('foo' => 'bar'));
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));

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
