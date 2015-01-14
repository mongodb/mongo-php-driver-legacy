--TEST--
Database: Profiling (turning on and off)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/collection-info.inc";
$a = mongo_standalone();

$d = $a->selectDb(dbname());
// Make sure it didn't exist from possibly previous bad runs
$d->dropCollection('system.profile');

$sp = $d->createCollection("system.profile", array('size' => 4096, 'capped' => true));

$retval = findCollection($d, 'system.profile');
var_dump($retval['name']);
dump_these_keys($retval['options'], array('size', 'capped'));

$d->setProfilingLevel(MongoDB::PROFILING_ON);

// we shouldn't be able to drop the collection now it seems
$d->dropCollection('system.profile');
$retval = findCollection($d, 'system.profile');
var_dump($retval['name']);
dump_these_keys($retval['options'], array('size', 'capped'));

// turn off profiling so we can drop the collection
$prev = $d->setProfilingLevel(MongoDB::PROFILING_OFF);

$d->dropCollection('system.profile');
var_dump(findCollection($d, 'system.profile'));
?>
--EXPECT--
string(14) "system.profile"
array(2) {
  ["size"]=>
  int(4096)
  ["capped"]=>
  bool(true)
}
string(14) "system.profile"
array(2) {
  ["size"]=>
  int(4096)
  ["capped"]=>
  bool(true)
}
NULL
