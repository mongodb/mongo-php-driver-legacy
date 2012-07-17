--TEST--
MongoRegex::__toString()
--SKIPIF--
<?php require __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
$regex = new MongoRegex('/foo[bar]{3}/imx');
echo (string) $regex . "\n";
?>
--EXPECT--
/foo[bar]{3}/imx
