--TEST--
Database: Dropping collections (memory test)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// drop the collection 100 times
$u = memory_get_usage(true);
for ($i = 0; $i < 100; $i++) {
	$d->dropCollection($d->dropcoltest);
}
var_dump($u - memory_get_usage(true));

// drop the collection 100 times
$u = memory_get_usage(true);
for ($i = 0; $i < 100; $i++) {
	$d->dropCollection('dropcoltest');
}
var_dump($u - memory_get_usage(true));

?>
--EXPECT--
int(0)
int(0)
