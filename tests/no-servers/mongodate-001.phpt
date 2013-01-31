--TEST--
MongoDate constructor uses current time
--DESCRIPTION--
This test purposesly allows for one second of variation between MongoDate and
the asserted timestamp, since we cannot guarantee that the clock will not
advance to the next second during test execution.
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$date = new MongoDate();
var_dump(time() - $date->sec <= 1);
var_dump($date->usec);
?>
--EXPECTF--
bool(true)
int(%d)
