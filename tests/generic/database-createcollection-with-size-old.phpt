--TEST--
Database: Create collection with max size (old)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// cleanup
$d->dropCollection('create-col1');
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

$c = $d->createCollection('create-col1', true, 4096);
$retval = $ns->findOne(array('name' => 'phpunit.create-col1'));
var_dump($retval['name']);
dump_these_keys($retval['options'], array('size', 'capped'));

// test cap
for ($i = 0; $i < 100; $i++) {
	$c->insert(array('x' => $i));
}
var_dump($c->count());
var_dump($c->count() < 125); // 4096 / (33-byte BSON documents) = 124.12
?>
--EXPECTF--
NULL

%s: MongoDB::createCollection(): This method now accepts arguments as an options array instead of the three optional arguments for capped, size and max elements in %sdatabase-createcollection-with-size-old.php on line %d
string(19) "phpunit.create-col1"
array(2) {
  ["size"]=>
  int(4096)
  ["capped"]=>
  bool(true)
}
int(%d)
bool(true)
