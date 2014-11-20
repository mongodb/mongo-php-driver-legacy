--TEST--
bson_encode() MongoDate
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

// Seconds only
$sec = 1416266917;
$usec = 0;
$hex = bin2hex(bson_encode(new MongoDate($sec, $usec)));
printf("%d.%06d: %s\n", $sec, $usec, $hex);

// Microseconds below precision
$sec = 1416266917;
$usec = 999;
$hex = bin2hex(bson_encode(new MongoDate($sec, $usec)));
printf("%d.%06d: %s\n", $sec, $usec, $hex);

// Microseconds within precision
$sec = 1416266917;
$usec = 555999;
$hex = bin2hex(bson_encode(new MongoDate($sec, $usec)));
printf("%d.%06d: %s\n", $sec, $usec, $hex);

?>
===DONE===
--EXPECT--
1416266917.000000: 882416c049010000
1416266917.000999: 882416c049010000
1416266917.555999: b32616c049010000
===DONE===
