--TEST--
Database: Create collection (errors)
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// cleanup
$d->dropCollection('create-col1');
$retval = $ns->findOne(array('name' => 'phpunit.create-col1'));
var_dump($retval);

// create
$d->createCollection();
$d->createCollection(array());
$d->createCollection(fopen(__FILE__, 'r'));
?>
--EXPECTF--
NULL

Warning: MongoDB::createCollection() expects at least 1 parameter, 0 given in %sdatabase-createcollection_error.php on line %d

Warning: MongoDB::createCollection() expects parameter 1 to be string, array given in %sdatabase-createcollection_error.php on line %d

Warning: MongoDB::createCollection() expects parameter 1 to be string, resource given in %sdatabase-createcollection_error.php on line %d
