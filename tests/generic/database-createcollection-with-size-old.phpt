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

// create
// * even though we're only setting this to 100, it allocates 1 extent, so we
//   can fit 4096, not 100, bytes of data in the collection.

$c = $d->createCollection('create-col1', true, 100);
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

// test cap
for ($i = 0; $i < 100; $i++) {
	$c->insert(array('x' => $i));
}
var_dump($c->count());
var_dump($c->count() < 100);
?>
--EXPECTF--
NULL

%s: MongoDB::createCollection(): This method now accepts arguments as an options array instead of the three optional arguments for capped, size and max elements in %sdatabase-createcollection-with-size-old.php on line %d
array(2) {
  ["name"]=>
  string(19) "phpunit.create-col1"
  ["options"]=>
  array(%d) {%A
    ["size"]=>
    int(100)
    ["capped"]=>
    bool(true)
  }
}
int(%d)
bool(true)
