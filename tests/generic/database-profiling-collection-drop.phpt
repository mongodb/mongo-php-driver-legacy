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

$sp = $d->createCollection("system.profile", array('size' => 4096, 'capped' => true));

$retval = $ns->findOne(array('name' => 'phpunit.system.profile'));
var_dump($retval['name']);
dump_these_keys($retval['options'], array('size', 'capped'));

$d->setProfilingLevel(MongoDB::PROFILING_ON);

// we shouldn't be able to drop the collection now it seems
$d->dropCollection('system.profile');
$retval = $ns->findOne(array('name' => 'phpunit.system.profile'));
var_dump($retval['name']);
dump_these_keys($retval['options'], array('size', 'capped'));

// turn off profiling so we can drop the collection
$prev = $d->setProfilingLevel(MongoDB::PROFILING_OFF);

$d->dropCollection('system.profile');
var_dump($ns->findOne(array('name' => 'phpunit.system.profile')));
?>
--EXPECTF--
string(22) "phpunit.system.profile"
array(2) {
  ["size"]=>
  int(4096)
  ["capped"]=>
  bool(true)
}
string(22) "phpunit.system.profile"
array(2) {
  ["size"]=>
  int(4096)
  ["capped"]=>
  bool(true)
}
NULL
