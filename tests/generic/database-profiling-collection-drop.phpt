--TEST--
Database: Profiling (turning on and off)
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$a = mongo();
$d = $a->selectDb("phpunit");
$ns = $d->selectCollection('system.namespaces');

$sp = $d->createCollection("system.profile", true, 5000);

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
--EXPECT--
array(2) {
  ["name"]=>
  string(22) "phpunit.system.profile"
  ["options"]=>
  array(3) {
    ["create"]=>
    string(14) "system.profile"
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
  array(3) {
    ["create"]=>
    string(14) "system.profile"
    ["size"]=>
    int(5000)
    ["capped"]=>
    bool(true)
  }
}
NULL
