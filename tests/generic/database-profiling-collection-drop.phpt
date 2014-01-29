--TEST--
Database: Profiling (turning on and off)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');
// Make sure it didn't exist from possibly previous bad runs
$d->dropCollection('system.profile');

$sp = $d->createCollection("system.profile", array('size' => 5000, 'capped' => true));

var_dump($ns->findOne(array('name' => 'phpunit.system.profile')));

$d->setProfilingLevel(MongoDB::PROFILING_ON);

// we shouldn't be able to drop the collection now it seems
$d->dropCollection('system.profile');
var_dump($ns->findOne(array('name' => 'phpunit.system.profile')));

// turn off profiling so we can drop the collection
$prev = $d->setProfilingLevel(MongoDB::PROFILING_OFF);

$d->dropCollection('system.profile');
var_dump($ns->findOne(array('name' => 'phpunit.system.profile')));
?>
--EXPECTF--
array(2) {
  ["name"]=>
  string(22) "phpunit.system.profile"
  ["options"]=>
  array(%d) {%A
    ["size"]=>
    int(5000)
    ["capped"]=>
    bool(true)
  }
}
array(2) {
  ["name"]=>
  string(22) "phpunit.system.profile"
  ["options"]=>
  array(%d) {%A
    ["size"]=>
    int(5000)
    ["capped"]=>
    bool(true)
  }
}
NULL
