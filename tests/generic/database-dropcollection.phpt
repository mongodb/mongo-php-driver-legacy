--TEST--
Database: Dropping collections (name-as-string)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/collection-info.inc";

$a = mongo_standalone();
$d = $a->selectDb(dbname());
$c = $d->selectCollection("dropcoltest");

// create a collection by inserting a record
$c->insert(array('foo' => 'bar'));
dump_these_keys(findCollection($d, 'dropcoltest'), array("name"));

// drop the collection
$d->dropCollection('dropcoltest');
var_dump(findCollection($d, 'dropcoltest'));

// dropping the new non-existant collection
$d->dropCollection('dropcoltest');
var_dump(findCollection($d, 'dropcoltest'));
?>
--EXPECT--
array(1) {
  ["name"]=>
  string(11) "dropcoltest"
}
NULL
NULL
