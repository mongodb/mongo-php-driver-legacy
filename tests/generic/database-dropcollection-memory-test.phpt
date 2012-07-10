--TEST--
Database: Dropping collections (memory test)
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$a = mongo();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// drop the collection 1000 times
$u = memory_get_usage(true);
for ($i = 0; $i < 1000; $i++) {
	$d->drop($d->dropcoltest);
}
var_dump($u - memory_get_usage(true));

// drop the collection 1000 times
$u = memory_get_usage(true);
for ($i = 0; $i < 1000; $i++) {
	$d->drop('dropcoltest');
}
var_dump($u - memory_get_usage(true));

?>
--EXPECT--
int(0)
int(0)
