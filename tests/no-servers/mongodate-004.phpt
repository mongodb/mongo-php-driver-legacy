--TEST--
MongoDate constructor casts arguments to integers
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$date = new MongoDate(null, null);
printf("%d.%06d\n", $date->sec, $date->usec);

$date = new MongoDate('12345', '67890');
printf("%d.%06d\n", $date->sec, $date->usec);
?>
--EXPECT--
0.000000
12345.067000
