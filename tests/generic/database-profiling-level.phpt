--TEST--
Database: Profiling (turning on and off)
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb("phpunit");
$sp = $d->createCollection("system.profile", true, 5000);

$prev = $d->setProfilingLevel(MongoDB::PROFILING_ON);
$level = $d->getProfilingLevel();
var_dump($level);

$prev = $d->setProfilingLevel(MongoDB::PROFILING_SLOW);
$level = $d->getProfilingLevel();
var_dump($prev);
var_dump($level);

$prev = $d->setProfilingLevel(MongoDB::PROFILING_OFF);
$level = $d->getProfilingLevel();
var_dump($prev);
var_dump($level);

$prev = $d->setProfilingLevel(MongoDB::PROFILING_OFF);
var_dump($prev);
?>
--EXPECT--
int(2)
int(2)
int(1)
int(1)
int(0)
int(0)
