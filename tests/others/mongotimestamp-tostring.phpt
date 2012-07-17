--TEST--
MongoTimestamp::__toString()
--SKIPIF--
<?php require __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
$ts = new MongoTimestamp(12345);
echo (string) $ts . "\n";
?>
--EXPECT--
12345
