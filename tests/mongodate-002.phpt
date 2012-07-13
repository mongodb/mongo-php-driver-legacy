--TEST--
MongoDate constructor has millisecond precision
--FILE--
<?php
$date = new MongoDate();
var_dump($date->usec === ($date->usec / 1000) * 1000);
?>
--EXPECT--
bool(true)
