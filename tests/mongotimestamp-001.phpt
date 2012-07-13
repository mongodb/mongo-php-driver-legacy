--TEST--
MongoTimestamp constructor uses automatic increment
--FILE--
<?php
$ts = new MongoTimestamp(0, 0);
printf("Timestamp(%d, %d)\n", $ts->sec, $ts->inc);

$ts = new MongoTimestamp();
printf("Timestamp(%d, %d)\n", $ts->sec, $ts->inc);

$ts = new MongoTimestamp(12345, 67890);
printf("Timestamp(%d, %d)\n", $ts->sec, $ts->inc);

$ts = new MongoTimestamp(100);
printf("Timestamp(%d, %d)\n", $ts->sec, $ts->inc);

$ts = new MongoTimestamp();
printf("Timestamp(%d, %d)\n", $ts->sec, $ts->inc);
?>
--EXPECTF--
Timestamp(0, 0)
Timestamp(%d, 0)
Timestamp(12345, 67890)
Timestamp(100, 1)
Timestamp(%d, 2)
