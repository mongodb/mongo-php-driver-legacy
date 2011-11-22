--TEST--
Database: Create collection
--FILE--
<?php
$a = new Mongo("localhost");
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// cleanup
$d->drop('create-col1');
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

// create
$d->createCollection('create-col1');
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));
?>
--EXPECT--
NULL
array(2) {
  ["name"]=>
  string(19) "phpunit.create-col1"
  ["options"]=>
  array(1) {
    ["create"]=>
    string(11) "create-col1"
  }
}
