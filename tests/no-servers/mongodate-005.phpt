--TEST--
MongoDate comparison
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$date1 = new MongoDate(12345, 67890);
$date2 = new MongoDate(12345, 67890);
var_dump($date1 == $date2);
var_dump($date1 === $date2);
?>
--EXPECT--
bool(true)
bool(false)
