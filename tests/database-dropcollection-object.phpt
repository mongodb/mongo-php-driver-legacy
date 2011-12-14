--TEST--
Database: Dropping collections (object)
--FILE--
<?php
$a = new Mongo("localhost");
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// create a collection by inserting a record
$d->dropcoltest->insert(array('foo' => 'bar'));
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));

// drop the collection
$d->drop($d->dropcoltest);
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));

// dropping the new non-existant collection
$d->drop($d->dropcoltest);
var_dump($ns->findOne(array('name' => 'phpunit.dropcoltest')));
?>
--EXPECTF--
array(1) {
  ["name"]=>
  string(19) "phpunit.dropcoltest"
}
NULL
NULL
