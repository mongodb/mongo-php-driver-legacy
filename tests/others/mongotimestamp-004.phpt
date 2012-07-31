--TEST--
MongoTimestamp comparison
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$ts1 = new MongoTimestamp(60, 30);
$ts2 = new MongoTimestamp(60, 30);
var_dump($ts1 == $ts2);
var_dump($ts1 === $ts2);
?>
--EXPECT--
bool(true)
bool(false)
