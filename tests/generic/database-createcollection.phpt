--TEST--
Database: Create collection
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$a = mongo();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// cleanup
$d->drop('create-col1');
$retval = $ns->findOne(array('name' => 'phpunit.create-col1'));
var_dump($retval);

// create
$d->createCollection('create-col1');
$retval = $ns->findOne(array('name' => 'phpunit.create-col1'));

var_dump($retval);
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
