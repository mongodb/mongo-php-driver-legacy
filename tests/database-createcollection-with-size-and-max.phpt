--TEST--
Database: Create collection with max size and items
--FILE--
<?php
$a = new Mongo("localhost");
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

// cleanup
$d->drop('create-col1');
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

// create
$c = $d->createCollection('create-col1', true, 1000, 5);
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

// test cap
for ($i = 0; $i < 10; $i++) {
	$c->insert(array('x' => $i));
}
var_dump($c->count());
?>
--EXPECT--
NULL
array(2) {
  ["name"]=>
  string(19) "phpunit.create-col1"
  ["options"]=>
  array(4) {
    ["create"]=>
    string(11) "create-col1"
    ["size"]=>
    int(1000)
    ["capped"]=>
    bool(true)
    ["max"]=>
    int(5)
  }
}
int(5)
