--TEST--
MongoTimestamp constructor casts arguments to integers
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$ts = new MongoTimestamp('60', '30');
var_dump($ts->sec);
var_dump($ts->inc);

$ts = new MongoTimestamp(60.123, 3e1);
var_dump($ts->sec);
var_dump($ts->inc);
?>
--EXPECT--
int(60)
int(30)
int(60)
int(30)
