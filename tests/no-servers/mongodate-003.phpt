--TEST--
MongoDate constructor with custom arguments
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$date = new MongoDate(0, 0);
printf("%d.%06d\n", $date->sec, $date->usec);

$date = new MongoDate(12345, 67890);
printf("%d.%06d\n", $date->sec, $date->usec);

$date = new MongoDate(12345);
printf("%d.%06d\n", $date->sec, $date->usec);
?>
--EXPECT--
0.000000
12345.067000
12345.000000
