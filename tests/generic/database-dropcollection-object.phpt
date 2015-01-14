--TEST--
Database: Dropping collections (object)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/collection-info.inc";

$a = mongo_standalone();
$d = $a->selectDb(dbname());

// create a collection by inserting a record
$d->dropcoltest->insert(array('foo' => 'bar'));
dump_these_keys(findCollection($d, 'dropcoltest'), array("name"));

// drop the collection
$d->dropCollection($d->dropcoltest);
var_dump(findCollection($d, 'dropcoltest'));

// dropping the new non-existant collection
$d->dropCollection($d->dropcoltest);
var_dump(findCollection($d, 'dropcoltest'));
?>
--EXPECT--
array(1) {
  ["name"]=>
  string(11) "dropcoltest"
}
NULL
NULL
