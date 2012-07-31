--TEST--
MongoDate constructor given invalid arguments
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$ts = new MongoDate('foo', 1);
var_dump($ts->sec);
var_dump($ts->usec);

$ts = new MongoDate(1, 'foo');
var_dump($ts->sec);
var_dump($ts->usec);
?>
--EXPECTF--
Warning: MongoDate::__construct() expects parameter 1 to be long, string given in %s
int(0)
int(0)

Warning: MongoDate::__construct() expects parameter 2 to be long, string given in %s
int(0)
int(0)
