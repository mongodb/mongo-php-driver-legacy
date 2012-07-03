--TEST--
MongoTimestamp::__toString()
--FILE--
<?php
$ts = new MongoTimestamp(12345);
echo (string) $ts . "\n";
?>
--EXPECT--
12345
