--TEST--
Database: Create collection with max size
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

$c = $d->createCollection('create-col1', array('size' => 1024, 'capped' => true));
var_dump($ns->findOne(array('name' => 'phpunit.create-col1')));

// test cap
for ($i = 0; $i < 1024; $i++) {
	$c->insert(array('x' => $i));
}
var_dump($c->count());
var_dump($c->count() < 1024);
?>
--EXPECTF--
NULL
array(2) {
  ["name"]=>
  string(19) "phpunit.create-col1"
  ["options"]=>
  array(%d) {%A
    ["capped"]=>
    bool(true)%A
  }
}
int(%d)
bool(true)
