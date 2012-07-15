--TEST--
MongoDate constructor has millisecond precision
--SKIPIF--
<?php require __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
$date = new MongoDate();
var_dump($date->usec === ($date->usec / 1000) * 1000);
?>
--EXPECT--
bool(true)
