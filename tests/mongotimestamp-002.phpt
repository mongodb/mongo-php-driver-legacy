--TEST--
MongoTimestamp constructor uses current time
--DESCRIPTION--
This test purposesly allows for one second of variation between MongoTimestamp
and the asserted timestamp, since we cannot guarantee that the clock will not
advance to the next second during test execution.
--FILE--
<?php
$ts = new MongoTimestamp();
var_dump(time() - $ts->sec <= 1);
?>
--EXPECT--
bool(true)
